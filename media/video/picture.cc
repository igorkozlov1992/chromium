// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "media/video/picture.h"

namespace media {

PictureBuffer::PictureBuffer(int32_t id, const gfx::Size& size)
    : id_(id), size_(size) {}

PictureBuffer::PictureBuffer(int32_t id,
                             const gfx::Size& size,
                             const TextureIds& client_texture_ids)
    : id_(id), size_(size), client_texture_ids_(client_texture_ids) {
  DCHECK(!client_texture_ids_.empty());
}

PictureBuffer::PictureBuffer(int32_t id,
                             const gfx::Size& size,
                             const TextureIds& client_texture_ids,
                             const TextureIds& service_texture_ids)
    : id_(id),
      size_(size),
      client_texture_ids_(client_texture_ids),
      service_texture_ids_(service_texture_ids) {
  DCHECK(!service_texture_ids_.empty());
  // We either not have client texture ids at all, or if we do, then their
  // number must be the same as the number of service texture ids.
  DCHECK(client_texture_ids_.empty() ||
         client_texture_ids_.size() == service_texture_ids_.size());
}

PictureBuffer::PictureBuffer(int32_t id,
                             const gfx::Size& size,
                             const TextureIds& client_texture_ids,
                             const std::vector<gpu::Mailbox>& texture_mailboxes)
    : id_(id),
      size_(size),
      client_texture_ids_(client_texture_ids),
      texture_mailboxes_(texture_mailboxes) {
  DCHECK_EQ(client_texture_ids.size(), texture_mailboxes.size());
}

PictureBuffer::PictureBuffer(const PictureBuffer& other) = default;

PictureBuffer::~PictureBuffer() {}

gpu::Mailbox PictureBuffer::texture_mailbox(size_t plane) const {
  if (plane >= texture_mailboxes_.size()) {
    LOG(ERROR) << "No mailbox for plane " << plane;
    return gpu::Mailbox();
  }

  return texture_mailboxes_[plane];
}

Picture::Picture(int32_t picture_buffer_id,
                 int32_t bitstream_buffer_id,
                 const gfx::Rect& visible_rect,
                 const gfx::ColorSpace& color_space,
                 bool allow_overlay)
    : picture_buffer_id_(picture_buffer_id),
      bitstream_buffer_id_(bitstream_buffer_id),
      visible_rect_(visible_rect),
      color_space_(color_space),
      allow_overlay_(allow_overlay),
      size_changed_(false),
      surface_texture_(false),
      wants_promotion_hint_(false) {}

}  // namespace media
