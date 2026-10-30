package com.kplayer.app;

import android.net.Uri;

final class MediaItem {
    final Uri uri;
    final String title;
    final String artist;
    final String album;
    final String mime;
    final long size;
    final long duration;
    final boolean video;

    MediaItem(Uri uri, String title, String artist, String album, String mime,
              long size, long duration, boolean video) {
        this.uri = uri;
        this.title = clean(title, "Sem título");
        this.artist = clean(artist, "Artista desconhecido");
        this.album = clean(album, "Álbum desconhecido");
        this.mime = clean(mime, video ? "video" : "audio");
        this.size = size;
        this.duration = duration;
        this.video = video;
    }

    private static String clean(String value, String fallback) {
        return value == null || value.trim().isEmpty() ? fallback : value.trim();
    }
}
