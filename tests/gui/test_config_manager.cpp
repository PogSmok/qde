#include <gtest/gtest.h>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTemporaryDir>
#include <QTextStream>
#include "qde/gui/config.hpp"
#include "qde/gui/config_manager.hpp"

using namespace qde::gui::config;

inline ConfigKey<int> testConfig("test", "testField", 4, "Display Name",
                                 "Description",
                                 [](const int& v) { return v >= 1 && v <= 4; });

inline ConfigKey<QKeySequence> testConfigKeySequence(
    "keytest", "testShortcut", QKeySequence("Alt+Left"), "Display Name",
    "Description", shortcuts::shortcutValidator);

class ConfigManagerTest : public ::testing::Test {
 protected:
  std::unique_ptr<QTemporaryDir> tempDir;
  QString testConfigFilePath;
  QString testConfigKeySequenceFilePath;

  void SetUp() override {}

  void TearDown() override {
    if (tempDir == nullptr) {
      return;
    }
    tempDir.reset();
  }

  std::optional<QString> ReadFile(const QString& fileName) {
    QFile inputFile(fileName);

    if (!inputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qCritical() << "Error: Failed to open file for reading:"
                  << inputFile.errorString();
      return std::nullopt;
    }

    QTextStream in(&inputFile);
    QString fileContents = in.readAll();
    inputFile.close();
    return fileContents;
  }

  void DefaultValues() {
    testConfig.SetValue(4);
    testConfigKeySequence.SetValue(QKeySequence("Alt+Left"));
  }

  void PrepareValidConfig() {
    tempDir = std::make_unique<QTemporaryDir>();

    EXPECT_TRUE(tempDir->isValid())
        << "Failed to create a Qt temporary directory.";

    testConfigFilePath = QDir(tempDir->path()).filePath("test.json");
    {
      QFile configFile(testConfigFilePath);
      EXPECT_TRUE(configFile.open(QIODevice::WriteOnly | QIODevice::Text))
          << "Failed to open config.json for writing: "
          << configFile.errorString().toStdString();

      QTextStream out(&configFile);
      out << "{\n"
          << "    \"testField\": 2\n"
          << "}\n";

      configFile.close();
    }

    testConfigKeySequenceFilePath =
        QDir(tempDir->path()).filePath("keytest.json");
    {
      QFile configFile(testConfigKeySequenceFilePath);
      EXPECT_TRUE(configFile.open(QIODevice::WriteOnly | QIODevice::Text))
          << "Failed to open config.json for writing: "
          << configFile.errorString().toStdString();

      QTextStream out(&configFile);
      out << "{\n"
          << "    \"testShortcut\": \"Alt+Right\"\n"
          << "}\n";

      configFile.close();
    }
  }

  void PrepareInvalidConfig_BadValue() {
    tempDir = std::make_unique<QTemporaryDir>();

    EXPECT_TRUE(tempDir->isValid())
        << "Failed to create a Qt temporary directory.";

    testConfigFilePath = QDir(tempDir->path()).filePath("test.json");
    {
      QFile configFile(testConfigFilePath);
      EXPECT_TRUE(configFile.open(QIODevice::WriteOnly | QIODevice::Text))
          << "Failed to open config.json for writing: "
          << configFile.errorString().toStdString();

      QTextStream out(&configFile);
      out << "{\n"
          << "    \"testField\": -2\n"  // invalid value
          << "}\n";

      configFile.close();
    }

    testConfigKeySequenceFilePath =
        QDir(tempDir->path()).filePath("keytest.json");
    {
      QFile configFile(testConfigKeySequenceFilePath);
      EXPECT_TRUE(configFile.open(QIODevice::WriteOnly | QIODevice::Text))
          << "Failed to open config.json for writing: "
          << configFile.errorString().toStdString();

      QTextStream out(&configFile);
      out << "{\n"
          << "    \"testShortcut\": \"Alt+Right\"\n"  // correct value
          << "}\n";

      configFile.close();
    }
  }

  void PrepareInvalidConfig_BadJson() {
    tempDir = std::make_unique<QTemporaryDir>();

    EXPECT_TRUE(tempDir->isValid())
        << "Failed to create a Qt temporary directory.";

    testConfigFilePath = QDir(tempDir->path()).filePath("test.json");
    {
      QFile configFile(testConfigFilePath);
      EXPECT_TRUE(configFile.open(QIODevice::WriteOnly | QIODevice::Text))
          << "Failed to open config.json for writing: "
          << configFile.errorString().toStdString();

      QTextStream out(&configFile);
      out << "{\n"
          << "    \"testField\": 2\n"
          << "\n";  // missing }

      configFile.close();
    }

    testConfigKeySequenceFilePath =
        QDir(tempDir->path()).filePath("keytest.json");
    {
      QFile configFile(testConfigKeySequenceFilePath);
      EXPECT_TRUE(configFile.open(QIODevice::WriteOnly | QIODevice::Text))
          << "Failed to open config.json for writing: "
          << configFile.errorString().toStdString();

      QTextStream out(&configFile);
      out << "{\n"
          << "    \"testShortcut\"; \"Alt+Right\"\n"  // bad syntax
          << "}\n";

      configFile.close();
    }
  }
};

TEST_F(ConfigManagerTest, LoadValidConfigFile) {
  PrepareValidConfig();
  DefaultValues();
  auto& mgr = ConfigManager::instance();

  EXPECT_EQ(testConfig.Value(), 4);
  EXPECT_EQ(testConfigKeySequence.Value(), QKeySequence("Alt+Left"));

  bool all_ok = mgr.load(tempDir->path());
  EXPECT_TRUE(all_ok);

  EXPECT_EQ(testConfig.Value(), 2);
  EXPECT_EQ(testConfigKeySequence.Value(), QKeySequence("Alt+Right"));
}

TEST_F(ConfigManagerTest, LoadConfigFileInvalidValue) {
  PrepareInvalidConfig_BadValue();
  DefaultValues();
  auto& mgr = ConfigManager::instance();

  EXPECT_EQ(testConfig.Value(), 4);
  EXPECT_EQ(testConfigKeySequence.Value(), QKeySequence("Alt+Left"));

  bool all_ok = mgr.load(tempDir->path());
  EXPECT_FALSE(all_ok);

  EXPECT_EQ(testConfig.Value(), 4) << "Config file contains invalid value. "
                                      "Config key value should not be changed.";
  EXPECT_EQ(testConfigKeySequence.Value(), QKeySequence("Alt+Right"))
      << "This config file contains valid value. Config key value should be "
         "changed.";
  ;
}

TEST_F(ConfigManagerTest, LoadConfigFileInvalidJson) {
  PrepareInvalidConfig_BadJson();
  DefaultValues();
  auto& mgr = ConfigManager::instance();

  EXPECT_EQ(testConfig.Value(), 4);
  EXPECT_EQ(testConfigKeySequence.Value(), QKeySequence("Alt+Left"));

  bool all_ok = mgr.load(tempDir->path());
  EXPECT_FALSE(all_ok);

  EXPECT_EQ(testConfig.Value(), 4) << "Config file contains bad syntax. Config "
                                      "key value should not be changed.";
  EXPECT_EQ(testConfigKeySequence.Value(), QKeySequence("Alt+Left"))
      << "Config file contains json syntax error. Config key value should not "
         "be changed.";
  ;
}

TEST_F(ConfigManagerTest, SaveValidConfigKeys) {
  PrepareValidConfig();
  DefaultValues();
  auto& mgr = ConfigManager::instance();

  testConfig.SetValue(1);
  testConfigKeySequence.SetValue(QKeySequence("Alt+Right"));

  bool all_ok = mgr.save(tempDir->path());
  EXPECT_TRUE(all_ok);
  {
    auto testConfigFileContent = ReadFile(testConfigFilePath);
    EXPECT_NE(testConfigFileContent, std::nullopt);
    EXPECT_EQ(testConfigFileContent, QString("{\n"
                                             "    \"testField\": 1\n"
                                             "}\n"));
  }
  {
    auto testConfigFileContent = ReadFile(testConfigKeySequenceFilePath);
    EXPECT_NE(testConfigFileContent, std::nullopt);
    EXPECT_EQ(testConfigFileContent,
              QString("{\n"
                      "    \"testShortcut\": \"Alt+Right\"\n"
                      "}\n"));
  }
}