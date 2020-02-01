/*
 * SPDX-FileCopyrightText: 2020 Vlad Zahorodnii <vladzzag@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "heifcreator.h"

#include <QImage>

#include <memory> // QScopedPointer is not a movable smart pointer :/

#include <libheif/heif.h>

extern "C"
{
    Q_DECL_EXPORT ThumbCreator *new_creator()
    {
        return new HeifCreator;
    }
}

struct HeifContextDeleter
{
    void operator()(heif_context *context) const
    {
        heif_context_free(context);
    }
};

struct HeifImageHandleDeleter
{
    void operator()(heif_image_handle *handle) const
    {
        heif_image_handle_release(handle);
    }
};

struct HeifImageDeleter
{
    void operator()(heif_image *image) const
    {
        heif_image_release(image);
    }
};

using scoped_heif_context_ptr = std::unique_ptr<heif_context, HeifContextDeleter>;
using scoped_heif_image_handle_ptr = std::unique_ptr<heif_image_handle, HeifImageHandleDeleter>;
using scoped_heif_image_ptr = std::unique_ptr<heif_image, HeifImageDeleter>;

HeifCreator::HeifCreator() = default;
HeifCreator::~HeifCreator() = default;

static scoped_heif_image_handle_ptr handleForPrimaryImage(heif_context *context)
{
    heif_image_handle *handle;

    const heif_error error = heif_context_get_primary_image_handle(context, &handle);
    if (error.code != heif_error_Ok) {
        return nullptr;
    }

    return scoped_heif_image_handle_ptr(handle);
}

static scoped_heif_image_handle_ptr handleForThumbnailImage(heif_image_handle *handle)
{
    heif_item_id id;
    heif_image_handle *thumbnailHandle;

    const int thumbnailCount = heif_image_handle_get_list_of_thumbnail_IDs(handle, &id, 1);
    if (!thumbnailCount) {
        return nullptr;
    }

    const heif_error error = heif_image_handle_get_thumbnail(handle, id, &thumbnailHandle);
    if (error.code != heif_error_Ok) {
        return nullptr;
    }

    return scoped_heif_image_handle_ptr(thumbnailHandle);
}

static scoped_heif_image_ptr imageForHandle(heif_image_handle *handle)
{
    heif_image *image;

    const heif_colorspace colorspace = heif_colorspace_RGB;
    const heif_chroma chroma = heif_chroma_interleaved_RGBA;

    const heif_error error = heif_decode_image(handle, &image, colorspace, chroma, nullptr);
    if (error.code != heif_error_Ok) {
        return nullptr;
    }

    return scoped_heif_image_ptr(image);
}

static scoped_heif_image_handle_ptr getThumbnailHandle(heif_context *context)
{
    scoped_heif_image_handle_ptr primaryImage = handleForPrimaryImage(context);
    if (!primaryImage) {
        return nullptr;
    }

    scoped_heif_image_handle_ptr thumbnailImage = handleForThumbnailImage(primaryImage.get());
    if (thumbnailImage) {
        return thumbnailImage;
    }

    return primaryImage;
}

bool HeifCreator::create(const QString &path, int width, int height, QImage &image)
{
    Q_UNUSED(width)
    Q_UNUSED(height)

    scoped_heif_context_ptr context(heif_context_alloc());
    if (!context) {
        return false;
    }

    const heif_error error = heif_context_read_from_file(context.get(), path.toUtf8(), nullptr);
    if (error.code != heif_error_Ok) {
        return false;
    }

    scoped_heif_image_handle_ptr thumbnailHandle = getThumbnailHandle(context.get());
    if (!thumbnailHandle) {
        return false;
    }

    scoped_heif_image_ptr thumbnailImage = imageForHandle(thumbnailHandle.get());
    if (!thumbnailImage) {
        return false;
    }

    const heif_channel channel = heif_channel_interleaved;

    const int thumbnailWidth = heif_image_get_width(thumbnailImage.get(), channel);
    const int thumbnailHeight = heif_image_get_height(thumbnailImage.get(), channel);
    if (thumbnailWidth <= 0 || thumbnailHeight <= 0) {
        return false;
    }

    int stride;
    const uint8_t *data = heif_image_get_plane_readonly(thumbnailImage.get(), channel, &stride);

    if (stride <= 0) {
        return false;
    }
    if (!data) {
        return false;
    }

    // Pixel data will be owned by QImage from now on.

    image = QImage(data, thumbnailWidth, thumbnailHeight, stride, QImage::Format_RGBA8888,
                   [](void *data) { heif_image_release(static_cast<heif_image *>(data)); },
                   thumbnailImage.release());

    return true;
}

ThumbCreator::Flags HeifCreator::flags() const
{
    return None;
}
