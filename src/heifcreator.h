/*
 * SPDX-FileCopyrightText: 2020 Vlad Zahorodnii <vladzzag@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <kio/thumbcreator.h>

class HeifCreator : public ThumbCreator
{
public:
    HeifCreator();
    ~HeifCreator() override;

    bool create(const QString &path, int width, int height, QImage &image) override;
    Flags flags() const override;
};
