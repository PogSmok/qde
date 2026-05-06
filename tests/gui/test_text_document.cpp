#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTextStream>
#include "qde/gui/text_document.hpp"

namespace qde::gui {

class TextDocumentTest : public ::testing::Test {
 protected:
  void SetUp() override { doc = new TextDocument(); }

  void TearDown() override { delete doc; }

 public:
  QPointer<TextDocument> doc;
};

TEST_F(TextDocumentTest, InitialState) {
  EXPECT_TRUE(doc->filePath().isEmpty());
  EXPECT_TRUE(doc->content().isEmpty());
  EXPECT_FALSE(doc->modified());
}

TEST_F(TextDocumentTest, SetContentEmitsSignals) {
  QSignalSpy content_spy(doc, &TextDocument::contentChanged);
  QSignalSpy modified_spy(doc, &TextDocument::modifiedChanged);

  doc->setContent("test content");

  EXPECT_EQ(doc->content(), "test content");
  EXPECT_TRUE(doc->modified());

  EXPECT_EQ(content_spy.count(), 1);
  EXPECT_EQ(modified_spy.count(), 1);
  // the argument should be true
  EXPECT_EQ(modified_spy.takeFirst().at(0).toBool(), true);
}

TEST_F(TextDocumentTest, LoadSave) {
  QTemporaryFile temp_file;
  ASSERT_TRUE(temp_file.open());

  QTextStream out(&temp_file);
  out << "original content";
  temp_file.close();

  std::filesystem::path path(temp_file.fileName().toStdString());

  QSignalSpy content_spy(doc, &TextDocument::contentChanged);
  QSignalSpy path_spy(doc, &TextDocument::filePathChanged);

  EXPECT_TRUE(doc->load(path));

  EXPECT_EQ(doc->content(), "original content");
  EXPECT_EQ(doc->filePath(), temp_file.fileName());
  EXPECT_FALSE(doc->modified());

  EXPECT_EQ(content_spy.count(), 1);
  EXPECT_EQ(path_spy.count(), 1);
  EXPECT_EQ(path_spy.takeFirst().at(0).toString(), temp_file.fileName());

  doc->setContent("new content");
  EXPECT_TRUE(doc->modified());

  EXPECT_TRUE(doc->save());
  EXPECT_FALSE(doc->modified());

  QFile read_back(temp_file.fileName());
  ASSERT_TRUE(read_back.open(QIODevice::ReadOnly | QIODevice::Text));
  QTextStream in(&read_back);
  EXPECT_EQ(in.readAll(), "new content");
}

}  // namespace qde::gui