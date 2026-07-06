#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDebug>
#include <QTextStream>
#include <QEventLoop>
#include <QByteArray>
#include <functional>

#include <qzstd.h>
#include <qsodiumkeys.h>
#include <qsodiumpasswordutils.h>
#include <qextrarandom.h>
#include <qsecuremem.h>

static const QString kPassText   = "secret_words";
static const QString kDataToTest = "HuggingFace Token: hf_ABC123XYZ789SecureManifestTokenData";
static const QString kKeysDir    = "testKeys";
static const QString ktestFile   = "test.txt";
static const QString kPubKeyName = "identity.pub";
static const QString kPriKeyName = "identity.key";

bool createDirFromFile(const QString &fileName)
{
    QFileInfo fi(fileName);
    QDir d = fi.absoluteDir();
    if (!d.exists())
        return d.mkpath(d.absolutePath());
    return true;
}

bool createDir(const QString &dirName)
{
    QDir d(dirName);
    if (!d.exists())
        return d.mkpath(dirName);
    return true;
}

bool writeTextFile(const QString &fileName, const QString &content)
{
    if (!createDirFromFile(fileName))
        return false;

    QFile f(fileName);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&f);
        out << content;
        f.flush();
        f.close();
        return f.error() == QFile::NoError;
    }
    return false;
}

QString readTextFile(const QString &fileName)
{
    QFile f(fileName);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        QString ret = in.readAll();
        f.close();
        return ret;
    }
    return {};
}

bool dirExists(const QString &path)
{
    return QFile::exists(path) && QFileInfo(path).isDir();
}

void setupWorkArea(const QString &tPath, const QString &kPath, const QString &compressDir)
{
    if (dirExists(tPath)) {
        QDir dir(tPath);
        dir.removeRecursively();
    }
    createDir(tPath);
    createDir(kPath);
    createDir(compressDir);
}

// Runs one QZstd operation to completion on a local event loop, adequate
// for a command line example. A real GUI app would connect to
// QZstd::finished directly and never block like this at all, the whole
// point of QZstd is to NOT need a loop like this in real usage.
bool runAndCheck(QZstd &engine, const std::function<void()> &trigger, const QString &label)
{
    QEventLoop loop;
    QObject::connect(&engine, &QZstd::finished, &loop, &QEventLoop::quit);

    trigger();
    loop.exec();

    if (!engine.get_errorString().isEmpty()) {
        qCritical() << "[-]" << label << "FAILED ->" << engine.get_errorString();
        return false;
    }

    qDebug() << "[+] TEST PASS:" << label;
    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QString baseTmp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (baseTmp.isEmpty())
        baseTmp = "/tmp";

    QString const testPath = baseTmp + "/qzstd-example";
    QString const keysPath = testPath + "/" + kKeysDir;
    QString const testFile = testPath + "/" + ktestFile;
    QString const testDir  = testPath + "/test_dir";

    setupWorkArea(testPath, keysPath, testDir);
    writeTextFile(testFile, kDataToTest);

    qDebug() << "=======================================================";
    qDebug() << " STARTING QZSTD (plain) TESTS";
    qDebug() << "=======================================================";

    {
        QZstd engine(nullptr);
        engine.set_input(testFile);
        engine.set_output(testPath + "/vanilla_test.txt.zst");
        engine.set_compressionLevel(5);

        qDebug() << "[+] Running plain async compression...";
        if (!runAndCheck(engine, [&]() { engine.compress(); }, "Plain compression"))
            return -1;

        engine.set_input(testPath + "/vanilla_test.txt.zst");
        engine.set_output(testPath + "/vanilla_extracted.txt");

        qDebug() << "[+] Running plain async decompression...";
        if (!runAndCheck(engine, [&]() { engine.decompress(); }, "Plain decompression"))
            return -1;

        QString const extractedContent = readTextFile(testPath + "/vanilla_extracted.txt");
        if (extractedContent != kDataToTest) {
            qCritical() << "[-] Plain round trip content mismatch.";
            return -1;
        }
        qDebug() << "[+] TEST PASS: Plain round trip content verified.";
    }

    qDebug() << "=======================================================";
    qDebug() << " STARTING QZSTD (signed + encrypted) TESTS";
    qDebug() << "=======================================================";

    // Signing keys are file based, QZstd's signing API only ever speaks in
    // paths, so generate a real Ed25519 keypair and save it to disk.
    QSodiumKeys signingKeys;
    if (!signingKeys.createKeys(QSodiumKeys::KeyType::Sign)) {
        qCritical() << "[-] Failed to generate Ed25519 signing keys.";
        return -1;
    }

    if (!signingKeys.saveKeys(keysPath, kPubKeyName, kPriKeyName)) {
        qCritical() << "[-] Failed to save signing keys to disk.";
        return -1;
    }

    QString const pubKeyFile = keysPath + "/" + kPubKeyName;
    QString const priKeyFile = keysPath + "/" + kPriKeyName;

    // The symmetric encryption key stays entirely in memory, password
    // derived, never written to a file.
    QByteArray const passBytes = kPassText.toUtf8();
    QSecureMem password(static_cast<size_t>(passBytes.size()));
    password.copyFrom(passBytes.constData(), static_cast<size_t>(passBytes.size()));

    QByteArray const salt = QExtraRandom::randomSalt();

    QSecureMem encryptionKey;
    if (!QSodiumPasswordUtils::deriveKeyFromPassword(encryptionKey, password, salt)) {
        qCritical() << "[-] Argon2id key derivation failed.";
        return -1;
    }

    {
        QString const copyTarget = testDir + "/" + ktestFile;
        writeTextFile(copyTarget, kDataToTest);

        QZstd engine(nullptr);
        engine.set_input(testDir);
        engine.set_output(testPath + "/secure_package.pkg");
        engine.set_compressionLevel(9);

        if (!engine.setPublicKeyFile(pubKeyFile) || !engine.setPrivateKeyFile(priKeyFile)) {
            qCritical() << "[-] Failed to load signing keys into QZstd.";
            return -1;
        }

        engine.setPrivateKey(encryptionKey);

        qDebug() << "[+] Running pipeline: compress -> encrypt -> sign...";
        if (!runAndCheck(engine, [&]() { engine.compress(true /* sign */, true /* encrypt */); }, "Compress + encrypt + sign"))
            return -1;

        QString const secureExtractOut = testPath + "/secure_extracted_dir";
        engine.set_input(testPath + "/secure_package.pkg");
        engine.set_output(secureExtractOut);

        qDebug() << "[+] Running pipeline: verify -> decrypt -> decompress...";
        if (!runAndCheck(engine, [&]() { engine.decompress(true /* verify */, true /* decrypt */); }, "Verify + decrypt + decompress"))
            return -1;

        QString const decryptedContent = readTextFile(secureExtractOut + "/" + ktestFile).trimmed();
        if (decryptedContent != kDataToTest.trimmed()) {
            qCritical() << "[-] Decrypted content mismatch.";
            qCritical() << "    Expected:" << kDataToTest;
            qCritical() << "    Got:     " << decryptedContent;
            return -1;
        }
        qDebug() << "[+] TEST PASS: Signed and encrypted round trip verified.";
    }

    qDebug() << "=======================================================";
    qDebug() << " ALL TESTS PASSED SUCCESSFULLY";
    qDebug() << "=======================================================";

    QDir(testPath).removeRecursively();
    return 0;
}