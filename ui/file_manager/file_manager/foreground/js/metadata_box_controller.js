// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Controller of metadata box.
 *
 * @param{!MetadataModel} metadataModel
 * @param{!FilesMetadataBox} metadataBox
 * @param{!FilesQuickView} quickView
 * @param{!QuickViewModel} quickViewModel
 * @param{!FileMetadataFormatter} fileMetadataFormatter
 *
 * @constructor
 */
function MetadataBoxController(
    metadataModel, metadataBox, quickView, quickViewModel,
    fileMetadataFormatter) {
  /**
   * @type {!MetadataModel}
   * @private
   */
  this.metadataModel_ = metadataModel;

  /**
   * @type {!FilesMetadataBox}
   * @private
   */
  this.metadataBox_ = metadataBox;

  /**
   * @type {!FilesQuickView}
   * @private
   */
  this.quickView_ = quickView;

  /**
   * @type {!FileMetadataFormatter}
   * @private
   */
  this.fileMetadataFormatter_ = fileMetadataFormatter;

  // TODO(oka): Add storage to persist the value of
  // quickViewModel_.metadataBoxActive.
  /**
   * @type {!QuickViewModel}
   * @private
   */
  this.quickViewModel_ = quickViewModel;

  fileMetadataFormatter.addEventListener(
      'date-time-format-changed', this.updateView_.bind(this));

  quickView.addEventListener(
      'metadata-box-active-changed', this.updateView_.bind(this));

  quickViewModel.addEventListener(
      'selected-entry-changed', this.updateView_.bind(this));
}

/**
 * @const {!Array<string>}
 */
MetadataBoxController.GENERAL_METADATA_NAME = [
  'size',
  'modificationTime',
];

/**
 * Update the view of metadata box.
 *
 * @private
 */
MetadataBoxController.prototype.updateView_ = function() {
  if (!this.quickView_.metadataBoxActive) {
    return;
  }
  this.metadataBox_.clear();
  var entry = this.quickViewModel_.getSelectedEntry();
  if (!entry)
    return;
  this.metadataModel_
      .get(
          [entry], MetadataBoxController.GENERAL_METADATA_NAME.concat(
                       ['hosted', 'externalFileUrl']))
      .then(this.onGeneralMetadataLoaded_.bind(this, entry));
};

/**
 * Update metadata box with general file information.
 * Then retrieve file specific metadata if any.
 *
 * @param {!Entry} entry
 * @param {!Array<!MetadataItem>} items
 *
 * @private
 */
MetadataBoxController.prototype.onGeneralMetadataLoaded_ = function(
    entry, items) {
  var type = FileType.getType(entry).type;
  var item = items[0];

  this.metadataBox_.type = type;
  if (item.size) {
    this.metadataBox_.size =
        this.fileMetadataFormatter_.formatSize(item.size, item.hosted);
  }
  if (entry.isDirectory) {
    this.setDirectorySize_( /** @type {!DirectoryEntry} */ (entry));
  }
  if (item.modificationTime) {
    this.metadataBox_.modificationTime =
        this.fileMetadataFormatter_.formatModDate(item.modificationTime);
  }

  if (item.externalFileUrl) {
    this.metadataModel_.get([entry], ['contentMimeType']).then(function(items) {
      var item = items[0];
      this.metadataBox_.mediaMimeType = item.contentMimeType;
    }.bind(this));
  } else {
    this.metadataModel_.get([entry], ['mediaMimeType']).then(function(items) {
      var item = items[0];
      this.metadataBox_.mediaMimeType = item.mediaMimeType;
    }.bind(this));
  }

  if (['image', 'video', 'audio'].includes(type)) {
    if (item.externalFileUrl) {
      this.metadataModel_.get([entry], ['imageHeight', 'imageWidth'])
          .then(function(items) {
            var item = items[0];
            this.metadataBox_.imageHeight = item.imageHeight;
            this.metadataBox_.imageWidth = item.imageWidth;
          }.bind(this));
    } else {
      this.metadataModel_
          .get(
              [entry],
              [
                'ifd',
                'imageHeight',
                'imageWidth',
                'mediaAlbum',
                'mediaArtist',
                'mediaDuration',
                'mediaGenre',
                'mediaTitle',
                'mediaTrack',
              ])
          .then(function(items) {
            var item = items[0];
            this.metadataBox_.ifd = item.ifd || null;
            this.metadataBox_.imageHeight = item.imageHeight || 0;
            this.metadataBox_.imageWidth = item.imageWidth || 0;
            this.metadataBox_.mediaAlbum = item.mediaAlbum || '';
            this.metadataBox_.mediaArtist = item.mediaArtist || '';
            this.metadataBox_.mediaDuration = item.mediaDuration || 0;
            this.metadataBox_.mediaGenre = item.mediaGenre || '';
            this.metadataBox_.mediaTitle = item.mediaTitle || '';
            this.metadataBox_.mediaTrack = item.mediaTrack || 0;
          }.bind(this));
    }
  }
};

/**
 * Set a current directory's size in metadata box.
 *
 * @param {!DirectoryEntry} entry
 *
 * @private
 */
MetadataBoxController.prototype.setDirectorySize_ = function(entry) {
  if (!entry.isDirectory)
    return;

  this.metadataBox_.isSizeLoading = true;
  chrome.fileManagerPrivate.getDirectorySize(entry,
      function(size) {
        if(this.quickViewModel_.getSelectedEntry() != entry) {
          return;
        }
        if(chrome.runtime.lastError) {
          this.metadataBox_.isSizeLoading = false;
          return;
        }

        this.metadataBox_.isSizeLoading = false;
        this.metadataBox_.size =
        this.fileMetadataFormatter_.formatSize(size, true);
      }.bind(this));
};
