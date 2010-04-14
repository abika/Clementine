/* This file is part of Clementine.

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "test_utils.h"
#include "gtest/gtest.h"

#include "playlist.h"
#include "mock_settingsprovider.h"
#include "mock_playlistitem.h"

#include <QtDebug>

using ::testing::Return;

namespace {

class PlaylistTest : public ::testing::Test {
 protected:
  PlaylistTest()
    : playlist_(NULL, new DummySettingsProvider),
      sequence_(NULL, new DummySettingsProvider)
  {
  }

  void SetUp() {
    playlist_.set_sequence(&sequence_);
  }

  MockPlaylistItem* MakeMockItem(const QString& title,
                                 const QString& artist = QString(),
                                 const QString& album = QString(),
                                 int length = 123) const {
    Song metadata;
    metadata.Init(title, artist, album, length);

    MockPlaylistItem* ret = new MockPlaylistItem;
    EXPECT_CALL(*ret, Metadata())
        .WillRepeatedly(Return(metadata));

    return ret;
  }

  boost::shared_ptr<PlaylistItem> MakeMockItemP(
      const QString& title, const QString& artist = QString(),
      const QString& album = QString(), int length = 123) const {
    return boost::shared_ptr<PlaylistItem>(MakeMockItem(title, artist, album, length));
  }

  Playlist playlist_;
  PlaylistSequence sequence_;
};

TEST_F(PlaylistTest, Basic) {
  EXPECT_EQ(0, playlist_.rowCount(QModelIndex()));
}

TEST_F(PlaylistTest, InsertItems) {
  MockPlaylistItem* item = MakeMockItem("Title", "Artist", "Album", 123);
  boost::shared_ptr<PlaylistItem> item_ptr(item);

  // Insert the item
  EXPECT_EQ(0, playlist_.rowCount(QModelIndex()));
  playlist_.InsertItems(PlaylistItemList() << item_ptr, -1);
  ASSERT_EQ(1, playlist_.rowCount(QModelIndex()));

  // Get the metadata
  EXPECT_EQ("Title", playlist_.data(playlist_.index(0, Playlist::Column_Title)));
  EXPECT_EQ("Artist", playlist_.data(playlist_.index(0, Playlist::Column_Artist)));
  EXPECT_EQ("Album", playlist_.data(playlist_.index(0, Playlist::Column_Album)));
  EXPECT_EQ(123, playlist_.data(playlist_.index(0, Playlist::Column_Length)));
}

TEST_F(PlaylistTest, Indexes) {
  playlist_.InsertItems(PlaylistItemList()
      << MakeMockItemP("One") << MakeMockItemP("Two") << MakeMockItemP("Three"));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  // Start "playing" track 1
  playlist_.set_current_index(0);
  EXPECT_EQ(0, playlist_.current_index());
  EXPECT_EQ("One", playlist_.current_item()->Metadata().title());
  EXPECT_EQ(-1, playlist_.previous_index());
  EXPECT_EQ(1, playlist_.next_index());

  // Stop playing
  EXPECT_EQ(0, playlist_.last_played_index());
  playlist_.set_current_index(-1);
  EXPECT_EQ(0, playlist_.last_played_index());
  EXPECT_EQ(-1, playlist_.current_index());

  // Play track 2
  playlist_.set_current_index(1);
  EXPECT_EQ(1, playlist_.current_index());
  EXPECT_EQ("Two", playlist_.current_item()->Metadata().title());
  EXPECT_EQ(0, playlist_.previous_index());
  EXPECT_EQ(2, playlist_.next_index());

  // Play track 3
  playlist_.set_current_index(2);
  EXPECT_EQ(2, playlist_.current_index());
  EXPECT_EQ("Three", playlist_.current_item()->Metadata().title());
  EXPECT_EQ(1, playlist_.previous_index());
  EXPECT_EQ(-1, playlist_.next_index());
}

TEST_F(PlaylistTest, RepeatPlaylist) {
  playlist_.InsertItems(PlaylistItemList()
      << MakeMockItemP("One") << MakeMockItemP("Two") << MakeMockItemP("Three"));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  playlist_.sequence()->SetRepeatMode(PlaylistSequence::Repeat_Playlist);

  playlist_.set_current_index(0);
  EXPECT_EQ(1, playlist_.next_index());

  playlist_.set_current_index(1);
  EXPECT_EQ(2, playlist_.next_index());

  playlist_.set_current_index(2);
  EXPECT_EQ(0, playlist_.next_index());
}

TEST_F(PlaylistTest, RepeatTrack) {
  playlist_.InsertItems(PlaylistItemList()
      << MakeMockItemP("One") << MakeMockItemP("Two") << MakeMockItemP("Three"));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  playlist_.sequence()->SetRepeatMode(PlaylistSequence::Repeat_Track);

  playlist_.set_current_index(0);
  EXPECT_EQ(0, playlist_.next_index());
}

TEST_F(PlaylistTest, RepeatAlbum) {
  playlist_.InsertItems(PlaylistItemList()
      << MakeMockItemP("One", "Album one")
      << MakeMockItemP("Two", "Album two")
      << MakeMockItemP("Three", "Album one"));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  playlist_.sequence()->SetRepeatMode(PlaylistSequence::Repeat_Album);

  playlist_.set_current_index(0);
  EXPECT_EQ(2, playlist_.next_index());

  playlist_.set_current_index(2);
  EXPECT_EQ(0, playlist_.next_index());
}

TEST_F(PlaylistTest, RemoveBeforeCurrent) {
  playlist_.InsertItems(PlaylistItemList()
      << MakeMockItemP("One") << MakeMockItemP("Two") << MakeMockItemP("Three"));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  // Remove a row before the currently playing track
  playlist_.set_current_index(2);
  EXPECT_EQ(2, playlist_.current_index());
  playlist_.removeRow(1, QModelIndex());
  EXPECT_EQ(1, playlist_.current_index());
  EXPECT_EQ(1, playlist_.last_played_index());
  EXPECT_EQ(0, playlist_.previous_index());
  EXPECT_EQ(-1, playlist_.next_index());
}

TEST_F(PlaylistTest, RemoveAfterCurrent) {
  playlist_.InsertItems(PlaylistItemList()
      << MakeMockItemP("One") << MakeMockItemP("Two") << MakeMockItemP("Three"));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  // Remove a row after the currently playing track
  playlist_.set_current_index(0);
  EXPECT_EQ(0, playlist_.current_index());
  playlist_.removeRow(1, QModelIndex());
  EXPECT_EQ(0, playlist_.current_index());
  EXPECT_EQ(0, playlist_.last_played_index());
  EXPECT_EQ(-1, playlist_.previous_index());
  EXPECT_EQ(1, playlist_.next_index());

  playlist_.set_current_index(1);
  EXPECT_EQ(-1, playlist_.next_index());
}

TEST_F(PlaylistTest, RemoveCurrent) {
  playlist_.InsertItems(PlaylistItemList()
      << MakeMockItemP("One") << MakeMockItemP("Two") << MakeMockItemP("Three"));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  // Remove the currently playing track's row
  playlist_.set_current_index(1);
  EXPECT_EQ(1, playlist_.current_index());
  playlist_.removeRow(1, QModelIndex());
  EXPECT_EQ(-1, playlist_.current_index());
  EXPECT_EQ(-1, playlist_.last_played_index());
  EXPECT_EQ(-1, playlist_.previous_index());
  EXPECT_EQ(0, playlist_.next_index());
}

TEST_F(PlaylistTest, InsertBeforeCurrent) {
  playlist_.InsertItems(PlaylistItemList()
      << MakeMockItemP("One") << MakeMockItemP("Two") << MakeMockItemP("Three"));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  playlist_.set_current_index(1);
  EXPECT_EQ(1, playlist_.current_index());
  playlist_.InsertItems(PlaylistItemList() << MakeMockItemP("Four"), 0);
  ASSERT_EQ(4, playlist_.rowCount(QModelIndex()));

  EXPECT_EQ(2, playlist_.current_index());
  EXPECT_EQ(2, playlist_.last_played_index());
  EXPECT_EQ(1, playlist_.previous_index());
  EXPECT_EQ(3, playlist_.next_index());

  EXPECT_EQ("Four", playlist_.data(playlist_.index(0, Playlist::Column_Title)));
  EXPECT_EQ("One", playlist_.data(playlist_.index(1, Playlist::Column_Title)));
}

TEST_F(PlaylistTest, InsertAfterCurrent) {
  playlist_.InsertItems(PlaylistItemList()
      << MakeMockItemP("One") << MakeMockItemP("Two") << MakeMockItemP("Three"));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  playlist_.set_current_index(1);
  EXPECT_EQ(1, playlist_.current_index());
  playlist_.InsertItems(PlaylistItemList() << MakeMockItemP("Four"), 2);
  ASSERT_EQ(4, playlist_.rowCount(QModelIndex()));

  EXPECT_EQ(1, playlist_.current_index());
  EXPECT_EQ(1, playlist_.last_played_index());
  EXPECT_EQ(0, playlist_.previous_index());
  EXPECT_EQ(2, playlist_.next_index());

  EXPECT_EQ("Two", playlist_.data(playlist_.index(1, Playlist::Column_Title)));
  EXPECT_EQ("Four", playlist_.data(playlist_.index(2, Playlist::Column_Title)));
  EXPECT_EQ("Three", playlist_.data(playlist_.index(3, Playlist::Column_Title)));
}

TEST_F(PlaylistTest, Clear) {
  playlist_.InsertItems(PlaylistItemList()
      << MakeMockItemP("One") << MakeMockItemP("Two") << MakeMockItemP("Three"));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  playlist_.set_current_index(1);
  EXPECT_EQ(1, playlist_.current_index());
  playlist_.Clear();

  EXPECT_EQ(0, playlist_.rowCount(QModelIndex()));
  EXPECT_EQ(-1, playlist_.current_index());
  EXPECT_EQ(-1, playlist_.last_played_index());
  EXPECT_EQ(-1, playlist_.previous_index());
  EXPECT_EQ(-1, playlist_.next_index());
}


} // namespace

