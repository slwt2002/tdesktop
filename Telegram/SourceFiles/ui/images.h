/*
This file is part of Telegram Desktop,
the official desktop version of Telegram messaging app, see https://telegram.org

Telegram Desktop is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

It is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

In addition, as a special exception, the copyright holders give permission
to link the code of portions of this program with the OpenSSL library.

Full license: https://github.com/telegramdesktop/tdesktop/blob/master/LICENSE
Copyright (c) 2014-2017 John Preston, https://desktop.telegram.org
*/
#pragma once

class FileLoader;
class mtpFileLoader;

enum LoadFromCloudSetting {
	LoadFromCloudOrLocal,
	LoadFromLocalOnly,
};

enum LoadToCacheSetting {
	LoadToFileOnly,
	LoadToCacheAsWell,
};

enum class ImageRoundRadius {
	None,
	Large,
	Small,
};
enum class ImageRoundCorner {
	None        = 0x00,
	TopLeft     = 0x01,
	TopRight    = 0x02,
	BottomLeft  = 0x04,
	BottomRight = 0x08,
	All         = 0x0f,
};
Q_DECLARE_FLAGS(ImageRoundCorners, ImageRoundCorner);
Q_DECLARE_OPERATORS_FOR_FLAGS(ImageRoundCorners);

inline uint32 packInt(int32 a) {
	return (a < 0) ? uint32(int64(a) + 0x100000000LL) : uint32(a);
}
inline int32 unpackInt(uint32 a) {
	return (a > 0x7FFFFFFFU) ? int32(int64(a) - 0x100000000LL) : int32(a);
}
inline uint64 packUIntUInt(uint32 a, uint32 b) {
	return (uint64(a) << 32) | uint64(b);
}
inline uint64 packUIntInt(uint32 a, int32 b) {
	return packUIntUInt(a, packInt(b));
}
inline uint64 packIntUInt(int32 a, uint32 b) {
	return packUIntUInt(packInt(a), b);
}
inline uint64 packIntInt(int32 a, int32 b) {
	return packUIntUInt(packInt(a), packInt(b));
}
inline uint32 unpackUIntFirst(uint64 v) {
	return uint32(v >> 32);
}
inline int32 unpackIntFirst(uint64 v) {
	return unpackInt(unpackUIntFirst(v));
}
inline uint32 unpackUIntSecond(uint64 v) {
	return uint32(v & 0xFFFFFFFFULL);
}
inline int32 unpackIntSecond(uint64 v) {
	return unpackInt(unpackUIntSecond(v));
}

class StorageImageLocation {
public:
	StorageImageLocation() : _widthheight(0), _dclocal(0), _volume(0), _secret(0) {
	}
	StorageImageLocation(int32 width, int32 height, int32 dc, const uint64 &volume, int32 local, const uint64 &secret) : _widthheight(packIntInt(width, height)), _dclocal(packIntInt(dc, local)), _volume(volume), _secret(secret) {
	}
	StorageImageLocation(int32 width, int32 height, const MTPDfileLocation &location) : _widthheight(packIntInt(width, height)), _dclocal(packIntInt(location.vdc_id.v, location.vlocal_id.v)), _volume(location.vvolume_id.v), _secret(location.vsecret.v) {
	}
	bool isNull() const {
		return !_dclocal;
	}
	int32 width() const {
		return unpackIntFirst(_widthheight);
	}
	int32 height() const {
		return unpackIntSecond(_widthheight);
	}
	void setSize(int32 width, int32 height) {
		_widthheight = packIntInt(width, height);
	}
	int32 dc() const {
		return unpackIntFirst(_dclocal);
	}
	uint64 volume() const {
		return _volume;
	}
	int32 local() const {
		return unpackIntSecond(_dclocal);
	}
	uint64 secret() const {
		return _secret;
	}

	static StorageImageLocation Null;

private:
	uint64 _widthheight;
	uint64 _dclocal;
	uint64 _volume;
	uint64 _secret;

	friend inline bool operator==(const StorageImageLocation &a, const StorageImageLocation &b) {
		return (a._dclocal == b._dclocal) && (a._volume == b._volume) && (a._secret == b._secret);
	}

};

inline bool operator!=(const StorageImageLocation &a, const StorageImageLocation &b) {
	return !(a == b);
}

namespace Images {

QImage prepareBlur(QImage image);
void prepareRound(QImage &image, ImageRoundRadius radius, ImageRoundCorners corners = ImageRoundCorner::All);
void prepareRound(QImage &image, QImage **cornerMasks, ImageRoundCorners corners = ImageRoundCorner::All);
void prepareCircle(QImage &image);
QImage prepareColored(style::color add, QImage image);
QImage prepareOpaque(QImage image);

enum class Option {
	None = 0x000,
	Smooth = 0x001,
	Blurred = 0x002,
	Circled = 0x004,
	RoundedLarge = 0x008,
	RoundedSmall = 0x010,
	RoundedTopLeft = 0x020,
	RoundedTopRight = 0x040,
	RoundedBottomLeft = 0x080,
	RoundedBottomRight = 0x100,
	Colored = 0x200,
};
Q_DECLARE_FLAGS(Options, Option);
Q_DECLARE_OPERATORS_FOR_FLAGS(Options);

QImage prepare(QImage img, int w, int h, Options options, int outerw, int outerh);

inline QPixmap pixmap(QImage img, int w, int h, Options options, int outerw, int outerh) {
	return QPixmap::fromImage(prepare(img, w, h, options, outerw, outerh), Qt::ColorOnly);
}

} // namespace Images

class DelayedStorageImage;

class HistoryItem;
class Image {
public:
	Image(const QString &file, QByteArray format = QByteArray());
	Image(const QByteArray &filecontent, QByteArray format = QByteArray());
	Image(const QPixmap &pixmap, QByteArray format = QByteArray());
	Image(const QByteArray &filecontent, QByteArray format, const QPixmap &pixmap);

	virtual void automaticLoad(const HistoryItem *item) { // auto load photo
	}
	virtual void automaticLoadSettingsChanged() {
	}

	virtual bool loaded() const {
		return true;
	}
	virtual bool loading() const {
		return false;
	}
	virtual bool displayLoading() const {
		return false;
	}
	virtual void cancel() {
	}
	virtual float64 progress() const {
		return 1;
	}
	virtual int32 loadOffset() const {
		return 0;
	}

	const QPixmap &pix(int32 w = 0, int32 h = 0) const;
	const QPixmap &pixRounded(int32 w = 0, int32 h = 0, ImageRoundRadius radius = ImageRoundRadius::None, ImageRoundCorners corners = ImageRoundCorner::All) const;
	const QPixmap &pixBlurred(int32 w = 0, int32 h = 0) const;
	const QPixmap &pixColored(style::color add, int32 w = 0, int32 h = 0) const;
	const QPixmap &pixBlurredColored(style::color add, int32 w = 0, int32 h = 0) const;
	const QPixmap &pixSingle(int32 w, int32 h, int32 outerw, int32 outerh, ImageRoundRadius radius, ImageRoundCorners corners = ImageRoundCorner::All) const;
	const QPixmap &pixBlurredSingle(int32 w, int32 h, int32 outerw, int32 outerh, ImageRoundRadius radius, ImageRoundCorners corners = ImageRoundCorner::All) const;
	const QPixmap &pixCircled(int32 w = 0, int32 h = 0) const;
	const QPixmap &pixBlurredCircled(int32 w = 0, int32 h = 0) const;
	QPixmap pixNoCache(int w = 0, int h = 0, Images::Options options = 0, int outerw = -1, int outerh = -1) const;
	QPixmap pixColoredNoCache(style::color add, int32 w = 0, int32 h = 0, bool smooth = false) const;
	QPixmap pixBlurredColoredNoCache(style::color add, int32 w, int32 h = 0) const;

	int32 width() const {
		return qMax(countWidth(), 1);
	}

	int32 height() const {
		return qMax(countHeight(), 1);
	}

	virtual void load(bool loadFirst = false, bool prior = true) {
	}

	virtual void loadEvenCancelled(bool loadFirst = false, bool prior = true) {
	}

	virtual const StorageImageLocation &location() const {
		return StorageImageLocation::Null;
	}

	bool isNull() const;

	void forget() const;

	QByteArray savedFormat() const {
		return _format;
	}
	QByteArray savedData() const {
		return _saved;
	}

	virtual DelayedStorageImage *toDelayedStorageImage() {
		return 0;
	}
	virtual const DelayedStorageImage *toDelayedStorageImage() const {
		return 0;
	}

	virtual ~Image();

protected:
	Image(QByteArray format = "PNG") : _format(format), _forgot(false) {
	}

	void restore() const;
	virtual void checkload() const {
	}
	void invalidateSizeCache() const;

	virtual int32 countWidth() const {
		restore();
		return _data.width();
	}

	virtual int32 countHeight() const {
		restore();
		return _data.height();
	}

	mutable QByteArray _saved, _format;
	mutable bool _forgot;
	mutable QPixmap _data;

private:
	using Sizes = QMap<uint64, QPixmap>;
	mutable Sizes _sizesCache;

};

typedef QPair<uint64, uint64> StorageKey;
inline uint64 storageMix32To64(int32 a, int32 b) {
	return (uint64(*reinterpret_cast<uint32*>(&a)) << 32) | uint64(*reinterpret_cast<uint32*>(&b));
}
inline StorageKey storageKey(int32 dc, const uint64 &volume, int32 local) {
	return StorageKey(storageMix32To64(dc, local), volume);
}
inline StorageKey storageKey(const MTPDfileLocation &location) {
	return storageKey(location.vdc_id.v, location.vvolume_id.v, location.vlocal_id.v);
}
inline StorageKey storageKey(const StorageImageLocation &location) {
	return storageKey(location.dc(), location.volume(), location.local());
}

class RemoteImage : public Image {
public:
	void automaticLoad(const HistoryItem *item); // auto load photo
	void automaticLoadSettingsChanged();

	bool loaded() const;
	bool loading() const {
		return amLoading();
	}
	bool displayLoading() const;
	void cancel();
	float64 progress() const;
	int32 loadOffset() const;

	void setData(QByteArray &bytes, const QByteArray &format = QByteArray());

	void load(bool loadFirst = false, bool prior = true);
	void loadEvenCancelled(bool loadFirst = false, bool prior = true);

	~RemoteImage();

protected:
	// If after loading the image we need to shrink it to fit into a
	// specific size, you can return this size here.
	virtual QSize shrinkBox() const {
		return QSize();
	}
	virtual void setInformation(int32 size, int32 width, int32 height) = 0;
	virtual FileLoader *createLoader(LoadFromCloudSetting fromCloud, bool autoLoading) = 0;

	void checkload() const {
		doCheckload();
	}
	void loadLocal();

private:
	mutable FileLoader *_loader = nullptr;
	bool amLoading() const;
	void doCheckload() const;

	void destroyLoaderDelayed(FileLoader *newValue = nullptr) const;

};

class StorageImage : public RemoteImage {
public:
	StorageImage(const StorageImageLocation &location, int32 size = 0);
	StorageImage(const StorageImageLocation &location, QByteArray &bytes);

	const StorageImageLocation &location() const override {
		return _location;
	}

protected:
	void setInformation(int32 size, int32 width, int32 height) override;
	FileLoader *createLoader(LoadFromCloudSetting fromCloud, bool autoLoading) override;

	StorageImageLocation _location;
	int32 _size;

	int32 countWidth() const override;
	int32 countHeight() const override;

};

class DelayedStorageImage : public StorageImage {
public:

	DelayedStorageImage();
	DelayedStorageImage(int32 w, int32 h);
	DelayedStorageImage(QByteArray &bytes);

	void setStorageLocation(const StorageImageLocation location);

	virtual DelayedStorageImage *toDelayedStorageImage() {
		return this;
	}
	virtual const DelayedStorageImage *toDelayedStorageImage() const {
		return this;
	}

	void automaticLoad(const HistoryItem *item); // auto load photo
	void automaticLoadSettingsChanged();

	bool loading() const {
		return _location.isNull() ? _loadRequested : StorageImage::loading();
	}
	bool displayLoading() const;
	void cancel();

	void load(bool loadFirst = false, bool prior = true);
	void loadEvenCancelled(bool loadFirst = false, bool prior = true);

private:
	bool _loadRequested, _loadCancelled, _loadFromCloud;

};

class WebImage : public RemoteImage {
public:

	// If !box.isEmpty() then resize the image to fit in this box.
	WebImage(const QString &url, QSize box = QSize());
	WebImage(const QString &url, int width, int height);

	void setSize(int width, int height);

protected:

	QSize shrinkBox() const override {
		return _box;
	}
	void setInformation(int32 size, int32 width, int32 height) override;
	FileLoader *createLoader(LoadFromCloudSetting fromCloud, bool autoLoading) override;

	int32 countWidth() const override;
	int32 countHeight() const override;

private:
	QString _url;
	QSize _box;
	int32 _size, _width, _height;

};

namespace internal {
	Image *getImage(const QString &file, QByteArray format);
	Image *getImage(const QString &url, QSize box);
	Image *getImage(const QString &url, int width, int height);
	Image *getImage(const QByteArray &filecontent, QByteArray format);
	Image *getImage(const QPixmap &pixmap, QByteArray format);
	Image *getImage(const QByteArray &filecontent, QByteArray format, const QPixmap &pixmap);
	Image *getImage(int32 width, int32 height);
	StorageImage *getImage(const StorageImageLocation &location, int32 size = 0);
	StorageImage *getImage(const StorageImageLocation &location, const QByteArray &bytes);
} // namespace internal

class ImagePtr : public ManagedPtr<Image> {
public:
	ImagePtr();
	ImagePtr(const QString &file, QByteArray format = QByteArray()) : Parent(internal::getImage(file, format)) {
	}
	ImagePtr(const QString &url, QSize box) : Parent(internal::getImage(url, box)) {
	}
	ImagePtr(const QString &url, int width, int height) : Parent(internal::getImage(url, width, height)) {
	}
	ImagePtr(const QByteArray &filecontent, QByteArray format = QByteArray()) : Parent(internal::getImage(filecontent, format)) {
	}
	ImagePtr(const QByteArray &filecontent, QByteArray format, const QPixmap &pixmap) : Parent(internal::getImage(filecontent, format, pixmap)) {
	}
	ImagePtr(const QPixmap &pixmap, QByteArray format) : Parent(internal::getImage(pixmap, format)) {
	}
	ImagePtr(const StorageImageLocation &location, int32 size = 0) : Parent(internal::getImage(location, size)) {
	}
	ImagePtr(const StorageImageLocation &location, const QByteArray &bytes) : Parent(internal::getImage(location, bytes)) {
	}
	ImagePtr(int32 width, int32 height, const MTPFileLocation &location, ImagePtr def = ImagePtr());
	ImagePtr(int32 width, int32 height) : Parent(internal::getImage(width, height)) {
	}

	explicit operator bool() const {
		return (_data != nullptr) && !_data->isNull();
	}

};

inline QSize shrinkToKeepAspect(int32 width, int32 height, int32 towidth, int32 toheight) {
	int32 w = qMax(width, 1), h = qMax(height, 1);
	if (w * toheight > h * towidth) {
		h = qRound(h * towidth / float64(w));
		w = towidth;
	} else {
		w = qRound(w * toheight / float64(h));
		h = toheight;
	}
	return QSize(qMax(w, 1), qMax(h, 1));
}

void clearStorageImages();
void clearAllImages();
int64 imageCacheSize();

class PsFileBookmark;
class ReadAccessEnabler {
public:
	ReadAccessEnabler(const PsFileBookmark *bookmark);
	ReadAccessEnabler(const QSharedPointer<PsFileBookmark> &bookmark);
	bool failed() const {
		return _failed;
	}
	~ReadAccessEnabler();

private:
	const PsFileBookmark *_bookmark;
	bool _failed;

};

class FileLocation {
public:
	FileLocation() = default;
	explicit FileLocation(const QString &name);

	bool check() const;
	const QString &name() const;
	void setBookmark(const QByteArray &bookmark);
	QByteArray bookmark() const;
	bool isEmpty() const {
		return name().isEmpty();
	}

	bool accessEnable() const;
	void accessDisable() const;

	QString fname;
	QDateTime modified;
	qint32 size;

private:
	QSharedPointer<PsFileBookmark> _bookmark;

};
inline bool operator==(const FileLocation &a, const FileLocation &b) {
	return (a.name() == b.name()) && (a.modified == b.modified) && (a.size == b.size);
}
inline bool operator!=(const FileLocation &a, const FileLocation &b) {
	return !(a == b);
}
