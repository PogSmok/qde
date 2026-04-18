#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTextStream>
#include "qde/qt/text_document.hpp"

class TextDocumentTest : public ::testing::Test {
protected:
 void SetUp() override {
  doc = new TextDocument();
 }

 void TearDown() override {
  delete doc;
 }

 TextDocument* doc;
};

TEST_F(TextDocumentTest, InitialState) {
 EXPECT_TRUE(doc->filePath().isEmpty());
 EXPECT_TRUE(doc->content().isEmpty());
 EXPECT_FALSE(doc->modified());
}

TEST_F(TextDocumentTest, SetContentEmitsSignals) {
 QSignalSpy contentSpy(doc, &TextDocument::contentChanged);
 QSignalSpy modifiedSpy(doc, &TextDocument::modifiedChanged);

 doc->setContent("test content");

 EXPECT_EQ(doc->content(), "test content");
 EXPECT_TRUE(doc->modified());
 
 EXPECT_EQ(contentSpy.count(), 1);
 EXPECT_EQ(modifiedSpy.count(), 1);
 // the argument should be true
 EXPECT_EQ(modifiedSpy.takeFirst().at(0).toBool(), true);
}

TEST_F(TextDocumentTest, LoadSave) {
 QTemporaryFile tempFile;
 ASSERT_TRUE(tempFile.open());
 
 QTextStream out(&tempFile);
 out << "original content";
 tempFile.close();

 std::filesystem::path path(tempFile.fileName().toStdString());

 QSignalSpy contentSpy(doc, &TextDocument::contentChanged);
 QSignalSpy pathSpy(doc, &TextDocument::filePathChanged);

 EXPECT_TRUE(doc->load(path));
 
 EXPECT_EQ(doc->content(), "original content");
 EXPECT_EQ(doc->filePath(), tempFile.fileName());
 EXPECT_FALSE(doc->modified());

 EXPECT_EQ(contentSpy.count(), 1);
 EXPECT_EQ(pathSpy.count(), 1);
 EXPECT_EQ(pathSpy.takeFirst().at(0).toString(), tempFile.fileName());

 doc->setContent("new content");
 EXPECT_TRUE(doc->modified());

 EXPECT_TRUE(doc->save());
 EXPECT_FALSE(doc->modified());

 QFile readBack(tempFile.fileName());
 ASSERT_TRUE(readBack.open(QIODevice::ReadOnly | QIODevice::Text));
 QTextStream in(&readBack);
 EXPECT_EQ(in.readAll(), "new content");
}