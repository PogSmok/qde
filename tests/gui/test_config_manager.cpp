#include <gtest/gtest.h>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTemporaryDir>
#include <QTextStream>
#include "qde/gui/config.hpp"
#include "qde/gui/config_manager.hpp"

using namespace qde::gui::config;

inline ConfigKey<int> test_config("test", "testField", 4, "Display Name",
                                  "Description", [](const int& v) {
                                    return v >= 1 && v <= 4;
                                  });

inline ConfigKey<QKeySequence> test_config_key_sequence(
    "keytest", "testShortcut", QKeySequence("Alt+Left"), "Display Name",
    "Description", shortcuts::ShortcutValidator);

class ConfigManagerTest : public ::testing::Test {
 protected:
  std::unique_ptr<QTemporaryDir> tempDir_;
  QString testConfigFilePath_;
  QString testConfigKeySequenceFilePath_;

  void SetUp() override {}

  void TearDown() override {
    if (tempDir_ == nullptr) {
      return;
    }
    tempDir_.reset();
  }

  static std::optional<QString> ReadFile(const QString& file_name) {
    QFile input_file(file_name);

    if (!input_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qCritical() << "Error: Failed to open file for reading:"
                  << input_file.errorString();
      return std::nullopt;
    }

    QTextStream in(&input_file);
    QString file_contents = in.readAll();
    input_file.close();
    return file_contents;
  }

  static void DefaultValues() {
    test_config.SetValue(4);
    test_config_key_sequence.SetValue(QKeySequence("Alt+Left"));
  }

  void PrepareValidConfig() {
    tempDir_ = std::make_unique<QTemporaryDir>();

    EXPECT_TRUE(tempDir_->isValid())
        << "Failed to create a Qt temporary directory.";

    testConfigFilePath_ = QDir(tempDir_->path()).filePath("test.json");
    {
      QFile config_file(testConfigFilePath_);
      EXPECT_TRUE(config_file.open(QIODevice::WriteOnly | QIODevice::Text))
          << "Failed to open config.json for writing: "
          << config_file.errorString().toStdString();

      QTextStream out(&config_file);
      out << "{\n"
          << "    \"testField\": 2\n"
          << "}\n";

      config_file.close();
    }

    testConfigKeySequenceFilePath_ =
        QDir(tempDir_->path()).filePath("keytest.json");
    {
      QFile config_file(testConfigKeySequenceFilePath_);
      EXPECT_TRUE(config_file.open(QIODevice::WriteOnly | QIODevice::Text))
          << "Failed to open config.json for writing: "
          << config_file.errorString().toStdString();

      QTextStream out(&config_file);
      out << "{\n"
          << "    \"testShortcut\": \"Alt+Right\"\n"
          << "}\n";

      config_file.close();
    }
  }

  void PrepareInvalidConfigBadValue() {
    tempDir_ = std::make_unique<QTemporaryDir>();

    EXPECT_TRUE(tempDir_->isValid())
        << "Failed to create a Qt temporary directory.";

    testConfigFilePath_ = QDir(tempDir_->path()).filePath("test.json");
    {
      QFile config_file(testConfigFilePath_);
      EXPECT_TRUE(config_file.open(QIODevice::WriteOnly | QIODevice::Text))
          << "Failed to open config.json for writing: "
          << config_file.errorString().toStdString();

      QTextStream out(&config_file);
      out << "{\n"
          << "    \"testField\": -2\n"  // invalid value
          << "}\n";

      config_file.close();
    }

    testConfigKeySequenceFilePath_ =
        QDir(tempDir_->path()).filePath("keytest.json");
    {
      QFile config_file(testConfigKeySequenceFilePath_);
      EXPECT_TRUE(config_file.open(QIODevice::WriteOnly | QIODevice::Text))
          << "Failed to open config.json for writing: "
          << config_file.errorString().toStdString();

      QTextStream out(&config_file);
      out << "{\n"
          << "    \"testShortcut\": \"Alt+Right\"\n"  // correct value
          << "}\n";

      config_file.close();
    }
  }

  void PrepareInvalidConfigBadJson() {
    tempDir_ = std::make_unique<QTemporaryDir>();

    EXPECT_TRUE(tempDir_->isValid())
        << "Failed to create a Qt temporary directory.";

    testConfigFilePath_ = QDir(tempDir_->path()).filePath("test.json");
    {
      QFile config_file(testConfigFilePath_);
      EXPECT_TRUE(config_file.open(QIODevice::WriteOnly | QIODevice::Text))
          << "Failed to open config.json for writing: "
          << config_file.errorString().toStdString();

      QTextStream out(&config_file);
      out << "{\n"
          << "    \"testField\": 2\n"
          << "\n";  // missing }

      config_file.close();
    }

    testConfigKeySequenceFilePath_ =
        QDir(tempDir_->path()).filePath("keytest.json");
    {
      QFile config_file(testConfigKeySequenceFilePath_);
      EXPECT_TRUE(config_file.open(QIODevice::WriteOnly | QIODevice::Text))
          << "Failed to open config.json for writing: "
          << config_file.errorString().toStdString();

      QTextStream out(&config_file);
      out << "{\n"
          << "    \"testShortcut\"; \"Alt+Right\"\n"  // bad syntax
          << "}\n";

      config_file.close();
    }
  }
};

TEST_F(ConfigManagerTest, LoadValidConfigFile) {
  PrepareValidConfig();
  DefaultValues();
  auto& mgr = ConfigManager::Instance();

  EXPECT_EQ(test_config.Value(), 4);
  EXPECT_EQ(test_config_key_sequence.Value(), QKeySequence("Alt+Left"));

  bool all_ok = mgr.Load(tempDir_->path());
  EXPECT_TRUE(all_ok);

  EXPECT_EQ(test_config.Value(), 2);
  EXPECT_EQ(test_config_key_sequence.Value(), QKeySequence("Alt+Right"));
}

TEST_F(ConfigManagerTest, LoadConfigFileInvalidValue) {
  PrepareInvalidConfigBadValue();
  DefaultValues();
  auto& mgr = ConfigManager::Instance();

  EXPECT_EQ(test_config.Value(), 4);
  EXPECT_EQ(test_config_key_sequence.Value(), QKeySequence("Alt+Left"));

  bool all_ok = mgr.Load(tempDir_->path());
  EXPECT_FALSE(all_ok);

  EXPECT_EQ(test_config.Value(), 4)
      << "Config file contains invalid value. "
         "Config key value should not be changed.";
  EXPECT_EQ(test_config_key_sequence.Value(), QKeySequence("Alt+Right"))
      << "This config file contains valid value. Config key value should be "
         "changed.";
  ;
}

TEST_F(ConfigManagerTest, LoadConfigFileInvalidJson) {
  PrepareInvalidConfigBadJson();
  DefaultValues();
  auto& mgr = ConfigManager::Instance();

  EXPECT_EQ(test_config.Value(), 4);
  EXPECT_EQ(test_config_key_sequence.Value(), QKeySequence("Alt+Left"));

  bool all_ok = mgr.Load(tempDir_->path());
  EXPECT_FALSE(all_ok);

  EXPECT_EQ(test_config.Value(), 4)
      << "Config file contains bad syntax. Config "
         "key value should not be changed.";
  EXPECT_EQ(test_config_key_sequence.Value(), QKeySequence("Alt+Left"))
      << "Config file contains json syntax error. Config key value should not "
         "be changed.";
  ;
}

TEST_F(ConfigManagerTest, SaveValidConfigKeys) {
  PrepareValidConfig();
  DefaultValues();
  auto& mgr = ConfigManager::Instance();

  test_config.SetValue(1);
  test_config_key_sequence.SetValue(QKeySequence("Alt+Right"));

  bool all_ok = mgr.Save(tempDir_->path());
  EXPECT_TRUE(all_ok);
  {
    auto test_config_file_content = ReadFile(testConfigFilePath_);
    EXPECT_NE(test_config_file_content, std::nullopt);
    EXPECT_EQ(test_config_file_content, QString("{\n"
                                                "    \"testField\": 1\n"
                                                "}\n"));
  }
  {
    auto test_config_file_content = ReadFile(testConfigKeySequenceFilePath_);
    EXPECT_NE(test_config_file_content, std::nullopt);
    EXPECT_EQ(test_config_file_content,
              QString("{\n"
                      "    \"testShortcut\": \"Alt+Right\"\n"
                      "}\n"));
  }
}