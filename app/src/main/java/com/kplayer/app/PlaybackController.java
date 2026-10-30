package com.kplayer.app;

import android.content.Context;
import android.net.Uri;
import android.view.SurfaceView;
import android.view.View;

import org.videolan.libvlc.LibVLC;
import org.videolan.libvlc.Media;
import org.videolan.libvlc.MediaPlayer;
import org.videolan.libvlc.interfaces.IVLCVout;

import java.util.ArrayList;

final class PlaybackController {
    interface Listener { void changed(); }
    private final LibVLC vlc;
    private final MediaPlayer player;
    private final SurfaceView surface;
    private final Listener listener;
    private String title = "Nada reproduzindo";

    PlaybackController(Context context, SurfaceView surface, Listener listener) {
        this.surface = surface;
        this.listener = listener;
        ArrayList<String> args = new ArrayList<>();
        args.add("--network-caching=1500");
        args.add("--audio-time-stretch");
        vlc = new LibVLC(context, args);
        player = new MediaPlayer(vlc);
        IVLCVout videoOut = player.getVLCVout();
        videoOut.setVideoView(surface);
        videoOut.attachViews();
        player.setEventListener(event -> listener.changed());
    }

    void play(MediaItem item) { title = item.title; playUri(item.uri, item.video); }
    void playRadio(String name, String url) { title = name; playUri(Uri.parse(url), false); }

    private void playUri(Uri uri, boolean video) {
        surface.setVisibility(video ? View.VISIBLE : View.GONE);
        Media media = new Media(vlc, uri);
        media.setHWDecoderEnabled(true, false);
        player.setMedia(media);
        media.release();
        player.play();
        listener.changed();
    }

    void toggle() { if (player.isPlaying()) player.pause(); else player.play(); listener.changed(); }
    void seek(long delta) { player.setTime(Math.max(0, player.getTime() + delta)); }
    boolean playing() { return player.isPlaying(); }
    long time() { return Math.max(0, player.getTime()); }
    long length() { return Math.max(0, player.getLength()); }
    String title() { return title; }
    void release() { player.stop(); player.getVLCVout().detachViews(); player.release(); vlc.release(); }
}
