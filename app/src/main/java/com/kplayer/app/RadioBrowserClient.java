package com.kplayer.app;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

final class RadioBrowserClient {
    interface Callback { void done(List<RadioStation> stations, String error); }

    static void search(String filter, Callback callback) {
        new Thread(() -> {
            List<RadioStation> stations = new ArrayList<>();
            String error = null;
            try {
                String query = "https://de1.api.radio-browser.info/json/stations/search?hidebroken=true&limit=80&order=clickcount&reverse=true";
                if (!filter.isEmpty()) query += "&tag=" + URLEncoder.encode(filter, StandardCharsets.UTF_8.name());
                HttpURLConnection c = (HttpURLConnection) new URL(query).openConnection();
                c.setRequestProperty("User-Agent", "KPlayer-Android/0.1");
                c.setConnectTimeout(8000); c.setReadTimeout(10000);
                StringBuilder json = new StringBuilder();
                try (BufferedReader r = new BufferedReader(new InputStreamReader(c.getInputStream()))) {
                    String line; while ((line = r.readLine()) != null) json.append(line);
                }
                JSONArray array = new JSONArray(json.toString());
                for (int i = 0; i < array.length(); i++) {
                    JSONObject o = array.getJSONObject(i);
                    String url = o.optString("url_resolved", o.optString("url"));
                    if (!url.isEmpty()) stations.add(new RadioStation(o.optString("name", "Rádio"),
                            o.optString("country"), o.optString("codec"), url,
                            o.optString("favicon"), o.optInt("bitrate")));
                }
            } catch (Exception e) { error = e.getMessage(); }
            callback.done(stations, error);
        }, "kplayer-radio").start();
    }
}
