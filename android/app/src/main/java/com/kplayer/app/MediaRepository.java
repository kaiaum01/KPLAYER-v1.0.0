package com.kplayer.app;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.MediaStore;

import androidx.documentfile.provider.DocumentFile;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

final class MediaRepository {
    interface Callback { void done(List<MediaItem> items); }

    static void loadDevice(Context context, Callback callback) {
        new Thread(() -> {
            List<MediaItem> result = new ArrayList<>();
            query(context.getContentResolver(), MediaStore.Audio.Media.EXTERNAL_CONTENT_URI, false, result);
            query(context.getContentResolver(), MediaStore.Video.Media.EXTERNAL_CONTENT_URI, true, result);
            callback.done(result);
        }, "kplayer-media-scan").start();
    }

    static void loadTree(Context context, Uri tree, Callback callback) {
        new Thread(() -> {
            List<MediaItem> result = new ArrayList<>();
            DocumentFile root = DocumentFile.fromTreeUri(context, tree);
            if (root != null) walk(root, result, 0);
            callback.done(result);
        }, "kplayer-folder-scan").start();
    }

    private static void query(ContentResolver resolver, Uri base, boolean video, List<MediaItem> out) {
        String[] columns = video ? new String[]{
                MediaStore.MediaColumns._ID, MediaStore.MediaColumns.DISPLAY_NAME,
                MediaStore.MediaColumns.MIME_TYPE, MediaStore.MediaColumns.SIZE,
                MediaStore.MediaColumns.DURATION
        } : new String[]{
                MediaStore.MediaColumns._ID, MediaStore.MediaColumns.DISPLAY_NAME,
                MediaStore.MediaColumns.MIME_TYPE, MediaStore.MediaColumns.SIZE,
                MediaStore.MediaColumns.DURATION, MediaStore.Audio.Media.ARTIST,
                MediaStore.Audio.Media.ALBUM
        };
        try (Cursor c = resolver.query(base, columns, MediaStore.MediaColumns.SIZE + ">?",
                new String[]{"51200"}, MediaStore.MediaColumns.DATE_MODIFIED + " DESC")) {
            if (c == null) return;
            int id = c.getColumnIndexOrThrow(MediaStore.MediaColumns._ID);
            int name = c.getColumnIndexOrThrow(MediaStore.MediaColumns.DISPLAY_NAME);
            int mime = c.getColumnIndexOrThrow(MediaStore.MediaColumns.MIME_TYPE);
            int size = c.getColumnIndexOrThrow(MediaStore.MediaColumns.SIZE);
            int duration = c.getColumnIndexOrThrow(MediaStore.MediaColumns.DURATION);
            int artist = c.getColumnIndex(MediaStore.Audio.Media.ARTIST);
            int album = c.getColumnIndex(MediaStore.Audio.Media.ALBUM);
            while (c.moveToNext()) {
                Uri uri = Uri.withAppendedPath(base, Long.toString(c.getLong(id)));
                out.add(new MediaItem(uri, c.getString(name), artist >= 0 ? c.getString(artist) : null,
                        album >= 0 ? c.getString(album) : null, c.getString(mime), c.getLong(size),
                        c.getLong(duration), video));
            }
        } catch (Exception ignored) { }
    }

    private static void walk(DocumentFile node, List<MediaItem> out, int depth) {
        if (depth > 12 || out.size() >= 4096) return;
        for (DocumentFile file : node.listFiles()) {
            if (file.isDirectory()) {
                walk(file, out, depth + 1);
            } else if (file.isFile() && file.length() >= 51200) {
                String type = file.getType();
                String name = file.getName();
                String low = name == null ? "" : name.toLowerCase(Locale.ROOT);
                boolean video = type != null && type.startsWith("video/");
                boolean audio = type != null && type.startsWith("audio/");
                if (!audio && !video) {
                    audio = low.matches(".*\\.(mp3|flac|ogg|wav|m4a|aac|opus)$");
                    video = low.matches(".*\\.(mp4|mkv|avi|mov|webm|m4v|mpg|mpeg|ts|flv|wmv)$");
                }
                if (audio || video) out.add(new MediaItem(file.getUri(), name, null, null, type,
                        file.length(), 0, video));
            }
            if (out.size() >= 4096) break;
        }
    }
}
