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
  EXPECT_TRUE(doc->FilePath().isEmpty());
  EXPECT_TRUE(doc->Content().isEmpty());
  EXPECT_FALSE(doc->Modified());
}

TEST_F(TextDocumentTest, SetContentEmitsSignals) {
  QSignalSpy content_spy(doc, &TextDocument::ContentChanged);
  QSignalSpy modified_spy(doc, &TextDocument::ModifiedChanged);

  doc->SetContent("test content");

  EXPECT_EQ(doc->Content(), "test content");
  EXPECT_TRUE(doc->Modified());

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

  QSignalSpy content_spy(doc, &TextDocument::ContentChanged);
  QSignalSpy path_spy(doc, &TextDocument::FilePathChanged);

  EXPECT_TRUE(doc->Load(path));

  EXPECT_EQ(doc->Content(), "original content");
  EXPECT_EQ(doc->FilePath(), temp_file.fileName());
  EXPECT_FALSE(doc->Modified());

  EXPECT_EQ(content_spy.count(), 1);
  EXPECT_EQ(path_spy.count(), 1);
  EXPECT_EQ(path_spy.takeFirst().at(0).toString(), temp_file.fileName());

  doc->SetContent("new content");
  EXPECT_TRUE(doc->Modified());

  EXPECT_TRUE(doc->Save());
  EXPECT_FALSE(doc->Modified());

  QFile read_back(temp_file.fileName());
  ASSERT_TRUE(read_back.open(QIODevice::ReadOnly | QIODevice::Text));
  QTextStream in(&read_back);
  EXPECT_EQ(in.readAll(), "new content");
}

}  // namespace qde::gui