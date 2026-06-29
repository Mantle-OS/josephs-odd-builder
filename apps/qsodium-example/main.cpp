#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QTextStream>

#include <qsodium.h>

static const QString kPass        = "secret_words";
static const QString kDataToTest  = "HuggingFace Token: hf_ABC123XYZ789SecureManifestTokenData";
static const QString kKeysDir     = "testKeys";
static const QString ktestFile    = "test.txt";

bool createDirFromFile(const QString &fileName) {
    QFileInfo fi(fileName);
    QDir d = fi.absoluteDir();
    if (!d.exists()) {
        return d.mkpath(d.absolutePath());
    }
    return true;
}

bool createDir(const QString &dirName) {
    QDir d(dirName);
    if (!d.exists()) {
        return d.mkpath(dirName);
    }
    return true;
}

bool writeTextFile(const QString &fileName, const QString &content) {
    if (!createDirFromFile(fileName)) return false;

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

QString readTextFile(const QString &fileName) {
    QFile f(fileName);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        QString ret = in.readAll();
        f.close();
        return ret;
    }
    return {};
}

bool dirExists(const QString &path) {
    return QFile::exists(path) && QFileInfo(path).isDir();
}

void setupWorkArea(const QString &tPath, const QString &kPath) {
    if (dirExists(tPath)) {
        QDir dir(tPath);
        dir.removeRecursively();
    }
    createDir(tPath);
    createDir(kPath);
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QString baseTmp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (baseTmp.isEmpty()) {
        baseTmp = "/tmp";
    }

    QString const testPath = baseTmp + "/qsodium-example";
    QString const keysPath = testPath + "/" + kKeysDir;
    QString const testFile = testPath + "/" + ktestFile;

    setupWorkArea(testPath, keysPath);

    QSodium sodiumEngine;
    if (!sodiumEngine.isInitialized()) {
        qCritical() << "[-] TEST FAIL: QSodium runtime failed to initialize.";
        return -1;
    }
    qDebug() << "[+] TEST PASS: QSodium Engine service online.";

    // Kews
    QSodiumCryptoSign signEngine;
    if (!signEngine.createKeys(QSodiumKeys::KeyType::Sign)) {
        qCritical() << "[-] TEST FAIL: Failed to generate Ed25519 signature keys.";
        return -1;
    }
    qDebug() << "[+] TEST PASS: Generated Ed25519 keypair successfully.";
    qDebug() << "    > Generated PublicKey (Base64):" << signEngine.publicKey();
    if (!writeTextFile(testFile, kDataToTest)) {
        qCritical() << "[-] TEST FAIL: Unable to write test harness file asset.";
        return -1;
    }

    // QString signatureBase64;
    if (!writeTextFile(testFile, kDataToTest)) {
        qCritical() << "[-] TEST FAIL: Unable to write test harness file asset.";
        return -1;
    }

    QString signatureBase64;
    // Fixed: Signature matching exactly your 2-argument member function layout
    if (!signEngine.signFile(testFile, signatureBase64)) {
        qCritical() << "[-] TEST FAIL: Digital signature calculation loop failed.";
        return -1;
    }
    qDebug() << "[+] TEST PASS: Document streaming signature computed.";
    qDebug() << "    > Detached Signature (Base64):" << signatureBase64;



    // Run verification pass via service orchestrator API
    bool const sigValid = sodiumEngine.verifyFileSignature(testFile, signEngine.publicKey(), signatureBase64);
    if (!sigValid) {
        qCritical() << "[-] TEST FAIL: Core signature verification loop rejected authentic file.";
        return -1;
    }
    qDebug() << "[+] TEST PASS: Service Orchestrator verified file integrity.";


    QString const fileHashHex = sodiumEngine.computeFileBlake2b(testFile);
    if (fileHashHex.isEmpty()) {
        qCritical() << "[-] TEST FAIL: BLAKE2b processing stream failed.";
        return -1;
    }
    qDebug() << "[+] TEST PASS: BLAKE2b checksum calculated.";
    qDebug() << "    > Checksum (Hex): " << fileHashHex;

    QByteArray const randomSalt = QExtraRandom::randomSalt();
    QSecureMem derivedVaultKey;

    bool const keyDerived = QSodiumPasswordUtils::deriveKeyFromPassword(derivedVaultKey, kPass, randomSalt);
    if (!keyDerived || derivedVaultKey.size() != crypto_secretbox_KEYBYTES) {
        qCritical() << "[-] TEST FAIL: Argon2id hardware key derivation process failed.";
        return -1;
    }
    qDebug() << "[+] TEST PASS: Derived 256-bit secure key envelope out of user password entropy.";

    QByteArray cipherText;
    QByteArray nonce;
    QByteArray const plainTextBytes = kDataToTest.toUtf8();

    bool const encrypted = QSodium::encryptConfig(plainTextBytes, derivedVaultKey, cipherText, nonce);
    if (!encrypted) {
        qCritical() << "[-] TEST FAIL: Authenticated symmetric encryption box failed to lock.";
        return -1;
    }
    qDebug() << "[+] TEST PASS: Plaintext data successfully locked into authenticated container.";

    // Decrypt the payload back to verify matching states
    QSecureMem decryptedPayload;
    bool const decrypted = QSodium::decryptConfig(cipherText, derivedVaultKey, nonce, decryptedPayload);
    if (!decrypted) {
        qCritical() << "[-] TEST FAIL: Authentication MAC check failed or ciphertext corrupted.";
        return -1;
    }

    QString const decryptedStr = QString::fromUtf8(reinterpret_cast<const char*>(decryptedPayload.data()), decryptedPayload.size());
    if (decryptedStr != kDataToTest) {
        qCritical() << "[-] TEST FAIL: Decrypted matching mismatch! String integrity broke.";
        return -1;
    }

    qDebug() << "[+] TEST PASS: Symmetric SecretBox decryption verified payload authenticity.";
    qDebug() << "=======================================================";
    qDebug() << " ALL TESTS PASSED SUCCESSFULLY (0 ERRORS)              ";
    qDebug() << "=======================================================";

    // Clean up work area directory on clean exit state
    QDir(testPath).removeRecursively();

    // Zero arguments unit run completes immediately without hooking an active loop
    return 0;
}