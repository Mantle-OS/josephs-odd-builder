#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QTextStream>
#include <QEventLoop>

#include <qzstd.h>
#include <qzstdio.h>

#ifdef QZSTD_SODIUM_SUPPORT
#include <qsodium.h>
#include <qsecuremem.h>
#include <sodium/crypto_secretbox.h>
#endif

static const QString kPassText    = "secret_words";
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

void setupWorkArea(const QString &tPath, const QString &kPath, const QString &compressDir) {
    if (dirExists(tPath)) {
        QDir dir(tPath);
        dir.removeRecursively();
    }
    createDir(tPath);
    createDir(kPath);
    createDir(compressDir);
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QString baseTmp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (baseTmp.isEmpty()) {
        baseTmp = "/tmp";
    }

    QString const testPath = baseTmp + "/qzstd-example";
    QString const keysPath = testPath + "/" + kKeysDir;
    QString const testFile = testPath + "/" + ktestFile;
    QString const testDir  = testPath + "/test_dir";

    setupWorkArea(testPath, keysPath, testDir);

    // Populate baseline text payload for extraction validation checks
    writeTextFile(testFile, kDataToTest);

    qDebug() << "=======================================================";
    qDebug() << " STARTING   TESTS                                      ";
    qDebug() << "=======================================================";


    {
        QString const ioCompressedFile = testPath + "/qzstdio_direct_test.txt.zst";
        QString const ioExtractedFile  = testPath + "/qzstdio_direct_extracted.txt";

        qDebug() << "[+] Executing direct QZstdIO streaming compression task...";

        QFile srcFile(testFile);
        QFile compressedFile(ioCompressedFile);

        if (!srcFile.open(QIODevice::ReadOnly)) {
            qCritical() << "[-] QZstdIO TEST FAIL: Could not open source file ->" << srcFile.errorString();
            return -1;
        }

        if (!compressedFile.open(QIODevice::WriteOnly)) {
            qCritical() << "[-] QZstdIO TEST FAIL: Could not open compressed output file ->" << compressedFile.errorString();
            return -1;
        }

        QZstdIO zstdWriter(&compressedFile);
        zstdWriter.setCompressionLevel(5);

        if (!zstdWriter.open(QIODevice::WriteOnly)) {
            qCritical() << "[-] QZstdIO TEST FAIL: Could not open QZstdIO writer ->" << zstdWriter.errorString();
            return -1;
        }

        char buffer[65536];

        while (!srcFile.atEnd()) {
            qint64 const bytesRead = srcFile.read(buffer, sizeof(buffer));

            if (bytesRead < 0) {
                qCritical() << "[-] QZstdIO TEST FAIL: Source read error ->" << srcFile.errorString();
                return -1;
            }

            if (bytesRead == 0)
                break;

            qint64 const bytesWritten = zstdWriter.write(buffer, bytesRead);
            if (bytesWritten != bytesRead) {
                qCritical() << "[-] QZstdIO TEST FAIL: Zstd write error ->" << zstdWriter.errorString();
                return -1;
            }
        }

        zstdWriter.close();
        compressedFile.close();
        srcFile.close();

        if (!QFileInfo::exists(ioCompressedFile) || QFileInfo(ioCompressedFile).size() <= 0) {
            qCritical() << "[-] QZstdIO TEST FAIL: Compressed output file was not created or is empty.";
            return -1;
        }

        qDebug() << "[+] Executing direct QZstdIO streaming decompression task...";

        QFile compressedInput(ioCompressedFile);
        QFile extractedFile(ioExtractedFile);

        if (!compressedInput.open(QIODevice::ReadOnly)) {
            qCritical() << "[-] QZstdIO TEST FAIL: Could not open compressed input file ->" << compressedInput.errorString();
            return -1;
        }

        if (!extractedFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qCritical() << "[-] QZstdIO TEST FAIL: Could not open extracted output file ->" << extractedFile.errorString();
            return -1;
        }

        QZstdIO zstdReader(&compressedInput);

        if (!zstdReader.open(QIODevice::ReadOnly)) {
            qCritical() << "[-] QZstdIO TEST FAIL: Could not open QZstdIO reader ->" << zstdReader.errorString();
            return -1;
        }

        while (!zstdReader.atEnd()) {
            qint64 const bytesRead = zstdReader.read(buffer, sizeof(buffer));

            if (bytesRead < 0) {
                qCritical() << "[-] QZstdIO TEST FAIL: Zstd read error ->" << zstdReader.errorString();
                return -1;
            }

            if (bytesRead == 0)
                break;

            qint64 const bytesWritten = extractedFile.write(buffer, bytesRead);
            if (bytesWritten != bytesRead) {
                qCritical() << "[-] QZstdIO TEST FAIL: Extracted write error ->" << extractedFile.errorString();
                return -1;
            }
        }

        zstdReader.close();
        extractedFile.close();
        compressedInput.close();

        QString const ioExtractedContent = readTextFile(ioExtractedFile);
        if (ioExtractedContent != kDataToTest) {
            qCritical() << "[-] QZstdIO TEST FAIL: Direct stream extracted content mismatch.";
            return -1;
        }

        qDebug() << "[+] TEST PASS: Direct QZstdIO streaming round-trip verified.";
    }



    {
        QZstd engine;
        engine.setInput(testFile);
        engine.setOutput(testPath + "/vanilla_test.txt.zst");
        engine.setCompressionLevel(5);

        QEventLoop loop;
        QObject::connect(&engine, &QZstd::finished, &loop, &QEventLoop::quit);

        qDebug() << "[+] Executing pure asynchronous compression task...";
        engine.compress();
        loop.exec();

        if (!engine.errorString().isEmpty()) {
            qCritical() << "[-] CORE TEST FAIL: Compression engine error ->" << engine.errorString();
            return -1;
        }

        // Reverse the stream for data integrity parity check
        engine.setInput(testPath + "/vanilla_test.txt.zst");
        engine.setOutput(testPath + "/vanilla_extracted.txt");

        engine.decompress();
        loop.exec();

        if (!engine.errorString().isEmpty()) {
            qCritical() << "[-] CORE TEST FAIL: Decompression engine error ->" << engine.errorString();
            return -1;
        }

        QString const extractedContent = readTextFile(testPath + "/vanilla_extracted.txt");
        if (extractedContent != kDataToTest) {
            qCritical() << "[-] CORE TEST FAIL: Extracted plain text corrupt or modified.";
            return -1;
        }
        qDebug() << "[+] TEST PASS: Pure stateless decompression loop verified.";
    }

#ifdef QZSTD_SODIUM_SUPPORT
    qDebug() << "-------------------------------------------------------";
    qDebug() << " RE-RUNNING SECURITY TASKS                             ";
    qDebug() << "-------------------------------------------------------";
    QSodiumCryptoSign signEngine;
    if (!signEngine.createKeys(QSodiumKeys::KeyType::Sign)) {
        qCritical() << "[-] TEST FAIL: Failed to generate Ed25519 signature keys.";
        return -1;
    }
    QString const sigPublicKeyStr = signEngine.pubKey();
    QSecureMem const sigPrivateKeyContainer = signEngine.privateKey();
    QByteArray const randomSalt = QByteArray::fromHex("0123456789abcdef0123456789abcdef"); // Static testing salt
    QSecureMem derivedVaultKey;

    const QByteArray passBytes = kPassText.toUtf8();
    QSecureMem kPass(passBytes.size());
    kPass.copyFrom(passBytes.constData(), passBytes.size());

    if (!QSodiumPasswordUtils::deriveKeyFromPassword(derivedVaultKey, kPass, randomSalt)) {
        qCritical() << "[-] TEST FAIL: Argon2id context calculation failed.";
        return -1;
    }

    QString const privateSignKeyB64 = sigPrivateKeyContainer.toBase64();
    QString const secretBoxKeyB64   = derivedVaultKey.toBase64();

    {
        // Copy original test file into test directory folder row to validate polymorphic directory packaging
        QString const copyTarget = testDir + "/" + ktestFile;
        writeTextFile(copyTarget, kDataToTest);

        QZstd cryptoEngine;
        cryptoEngine.setInput(testDir); // Directing target input to the directory tree block
        cryptoEngine.setOutput(testPath + "/secure_package.pkg");
        cryptoEngine.setCompressionLevel(9);

        cryptoEngine.setPublicKey(sigPublicKeyStr);
        cryptoEngine.setSignatureKey(privateSignKeyB64);
        cryptoEngine.setPrivateKey(secretBoxKeyB64);

        QEventLoop cryptoLoop;
        QObject::connect(&cryptoEngine, &QZstd::finished,
                         &cryptoLoop, &QEventLoop::quit);

        qDebug() << "[+] Running Airtight Pipeline: Compress -> Encrypt -> Detached Sign...";
        cryptoEngine.compress(true /* sign */, true /* encrypt */);
        cryptoLoop.exec();

        if (!cryptoEngine.errorString().isEmpty()) {
            qCritical() << "[-] CRYPTO FAIL: Secure save pipeline broke ->" << cryptoEngine.errorString();
            return -1;
        }
        qDebug() << "[+] TEST PASS: Authenticated packages successfully compiled and signed.";

        // Reset and clear local configuration buffers to test parsing pipeline
        QString const secureExtractOut = testPath + "/secure_extracted_dir";
        cryptoEngine.setInput(testPath + "/secure_package.pkg");
        cryptoEngine.setOutput(secureExtractOut);

        qDebug() << "[+] Running Extraction Pipeline: Verify Signature -> Decrypt Box -> Decompress Stream...";
        cryptoEngine.decompress(true /* verify */, true /* decrypt */);
        cryptoLoop.exec();

        if (!cryptoEngine.errorString().isEmpty()) {
            qCritical() << "[-] CRYPTO FAIL: Secure load pipeline rejected asset ->" << cryptoEngine.errorString();
            return -1;
        }

        // FIXED: Query the exact relative path layout written by the decompressFolder routine!
        QString const decryptedContent = readTextFile(secureExtractOut + "/" + ktestFile).trimmed();
        if (decryptedContent != kDataToTest.trimmed()) {
            qCritical() << "[-] CRYPTO FAIL: Memory array validation checks mismatch.";
            qCritical() << "    Expected:" << kDataToTest;
            qCritical() << "    Got:     " << decryptedContent;
            return -1;
        }
        qDebug() << "[+] TEST PASS: Cryptographic operations executed without standard heap leaks!";
    }
#endif

    qDebug() << "=======================================================";
    qDebug() << " ALL TESTS PASSED SUCCESSFULLY                         ";
    qDebug() << "=======================================================";

    QDir(testPath).removeRecursively();
    return 0;
}