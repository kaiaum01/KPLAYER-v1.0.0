package com.kplayer.app;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.RectF;
import android.net.Uri;
import android.view.MotionEvent;
import android.view.View;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

final class KPlayerView extends View {
    private static final int BG = 0xff0b0e14, PANEL = 0xff151a24, BORDER = 0xff2b3342;
    private static final int TEXT = 0xfff5f7fb, MUTED = 0xff99a3b5, ACCENT = 0xff65e5a5;
    private final Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final SharedPreferences prefs;
    private PlaybackController playback;
    private Runnable folderPicker, permissionRequest;
    private final List<MediaItem> media = new ArrayList<>();
    private final List<RadioStation> stations = new ArrayList<>();
    private int page = 0, radioStart = 0;
    private boolean onboarding, acceptedTerms, showTerms, radioLoading;
    private String language, radioFilter = "";
    private MediaItem current;
    private RadioStation currentRadio;

    KPlayerView(Context context) {
        super(context);
        setLayerType(View.LAYER_TYPE_SOFTWARE, null);
        prefs = context.getSharedPreferences("kplayer", Context.MODE_PRIVATE);
        onboarding = !prefs.getBoolean("onboarded", false);
        acceptedTerms = prefs.getBoolean("terms", false);
        language = prefs.getString("lang", "pt-BR");
    }

    void bind(PlaybackController controller, Runnable folderPicker, Runnable permissionRequest) {
        playback = controller; this.folderPicker = folderPicker; this.permissionRequest = permissionRequest;
    }

    void scanDevice() {
        MediaRepository.loadDevice(getContext(), items -> post(() -> { media.clear(); media.addAll(items); invalidate(); }));
    }

    void scanFolder(Uri tree) {
        MediaRepository.loadTree(getContext(), tree, items -> post(() -> { media.clear(); media.addAll(items); page = 1; invalidate(); }));
    }

    private String tr(String pt, String en, String es) {
        if (language.startsWith("en")) return en;
        if (language.startsWith("es")) return es;
        return pt;
    }

    @Override protected void onDraw(Canvas c) {
        super.onDraw(c);
        if (page == 2 && current != null && current.video && playback != null && playback.playing()) {
            c.drawColor(0x00000000, PorterDuff.Mode.CLEAR);
            header(c, tr("Vídeo em reprodução", "Playing video", "Vídeo en reproducción"), current.title);
            drawNavigation(c);
            postInvalidateDelayed(800);
            return;
        }
        c.drawColor(BG);
        if (showTerms) drawTerms(c);
        else if (onboarding) drawOnboarding(c);
        else if (page == 0) drawDashboard(c);
        else if (page == 1) drawAudio(c);
        else if (page == 2) drawVideo(c);
        else drawRadio(c);
        if (!onboarding) drawNavigation(c);
        postInvalidateDelayed(800);
    }

    private void drawOnboarding(Canvas c) {
        float w = getWidth(), h = getHeight(), cx = w / 2f;
        text(c, "K", cx, h * .19f, Math.min(w, h) * .16f, ACCENT, Paint.Align.CENTER, true);
        text(c, "KPLAYER", cx, h * .31f, 29, TEXT, Paint.Align.CENTER, true);
        text(c, tr("Sua mídia. Seu controle.", "Your media. Your control.", "Tu contenido. Tu control."), cx, h * .37f, 16, MUTED, Paint.Align.CENTER, false);
        text(c, tr("Escolha o idioma", "Choose a language", "Elige el idioma"), cx, h * .47f, 17, TEXT, Paint.Align.CENTER, true);
        button(c, new RectF(cx-145, h*.51f, cx-50, h*.58f), "PT-BR", language.equals("pt-BR"));
        button(c, new RectF(cx-45, h*.51f, cx+45, h*.58f), "EN", language.equals("en"));
        button(c, new RectF(cx+50, h*.51f, cx+145, h*.58f), "ES", language.equals("es"));
        check(c, cx-145, h*.66f, acceptedTerms, tr("Li e aceito os termos de uso", "I accept the terms of use", "Acepto los términos de uso"));
        text(c, tr("TERMOS", "TERMS", "TÉRMINOS"), w-20, 34, 13, ACCENT, Paint.Align.RIGHT, true);
        button(c, new RectF(cx-145, h*.79f, cx+145, h*.88f), tr("INICIAR", "START", "INICIAR"), acceptedTerms);
    }

    private void drawTerms(Canvas c) {
        float w=getWidth(), h=getHeight();
        header(c,tr("Termos de uso", "Terms of use", "Términos de uso"),"KPlayer (Beta)");
        panel(c,new RectF(18,105,w-18,h-105));
        String[] lines={
                tr("O KPlayer é fornecido sem garantia.", "KPlayer is provided without warranty.", "KPlayer se proporciona sin garantía."),
                tr("Você é responsável pelas mídias reproduzidas.", "You are responsible for media you play.", "Eres responsable del contenido reproducido."),
                tr("Rádios pertencem aos seus operadores.", "Radio streams belong to their operators.", "Las radios pertenecen a sus operadores."),
                tr("O aplicativo não envia seus arquivos pessoais.", "The app does not upload your personal files.", "La aplicación no sube tus archivos personales."),
                tr("Toque abaixo para voltar.", "Tap below to return.", "Toca abajo para volver.")
        };
        float y=150;for(String line:lines){text(c,line,36,y,16,y>250?MUTED:TEXT,Paint.Align.LEFT,false);y+=46;}
        button(c,new RectF(36,h-172,w-36,h-120),tr("VOLTAR", "BACK", "VOLVER"),true);
    }

    private void drawDashboard(Canvas c) {
        header(c, tr("Central de mídia", "Media hub", "Centro multimedia"), tr("Biblioteca local e rádio online", "Local library and online radio", "Biblioteca local y radio online"));
        float w = getWidth(), top = 108, gap = 12, margin = 18;
        boolean portrait = getHeight() > getWidth();
        float cardW = portrait ? w-margin*2 : (w-margin*2-gap)/3;
        float cardH = portrait ? 112 : Math.min(180, getHeight()-190);
        String[] names = {tr("MÚSICAS", "MUSIC", "MÚSICA"), tr("VÍDEOS", "VIDEOS", "VÍDEOS"), tr("RÁDIO", "RADIO", "RADIO")};
        String[] subs = {count(false)+" faixas", count(true)+" vídeos", tr("Estações do mundo", "Stations worldwide", "Emisoras del mundo")};
        for (int i=0;i<3;i++) {
            float x = portrait ? margin : margin+i*(cardW+gap), y = portrait ? top+i*(cardH+gap) : top;
            card(c, new RectF(x,y,x+cardW,y+cardH), names[i], subs[i], i==2 ? "◉" : (i==1 ? "▶" : "♫"));
        }
        if (portrait) button(c,new RectF(margin,top+3*(cardH+gap)+4,w-margin,top+3*(cardH+gap)+58),tr("ESCOLHER PASTA", "CHOOSE FOLDER", "ELEGIR CARPETA"),true);
    }

    private void drawAudio(Canvas c) {
        boolean portrait = getHeight() > getWidth();
        header(c, tr("Músicas", "Music", "Música"), media.isEmpty()?tr("Permita o acesso ou escolha uma pasta", "Allow access or choose a folder", "Permite acceso o elige una carpeta"):media.size()+" itens");
        if (current == null) current = first(false);
        if (portrait) {
            float cx=getWidth()/2f;
            cover(c,new RectF(cx-105,105,cx+105,315));
            drawTrackInfo(c,current,22,344,getWidth()-44);
            controls(c,cx,465);
            drawQueue(c,22,525,getWidth()-44,getHeight()-605);
        } else {
            cover(c,new RectF(22,102,252,332));
            drawTrackInfo(c,current,276,112,getWidth()*.38f);
            controls(c,150,385);
            drawQueue(c,getWidth()*.58f,102,getWidth()*.39f,getHeight()-178);
        }
    }

    private void drawVideo(Canvas c) {
        header(c,tr("Vídeos", "Videos", "Vídeos"),tr("Toque para reproduzir em tela cheia", "Tap to play fullscreen", "Toca para reproducir"));
        int y=105, shown=0;
        for (int i=0;i<media.size() && shown<7;i++) if(media.get(i).video){
            MediaItem m=media.get(i); row(c,new RectF(18,y,getWidth()-18,y+58),m.title,formatSize(m.size)+" • "+m.mime,"▶"); y+=66; shown++;
        }
        if(shown==0) empty(c,tr("Nenhum vídeo encontrado", "No videos found", "No se encontraron vídeos"));
    }

    private void drawRadio(Canvas c) {
        boolean portrait=getHeight()>getWidth();
        header(c,tr("Rádio online", "Online radio", "Radio online"),radioFilter.isEmpty()?tr("Mais ouvidas", "Most popular", "Más populares"):radioFilter);
        float listX=portrait?16:getWidth()*.48f, listY=portrait?355:104, listW=portrait?getWidth()-32:getWidth()*.50f-16;
        float playerW=portrait?getWidth()-32:getWidth()*.44f;
        panel(c,new RectF(16,104,16+playerW,portrait?335:getHeight()-82));
        text(c,"◉",16+playerW/2,170,54,ACCENT,Paint.Align.CENTER,true);
        text(c,currentRadio==null?tr("Escolha uma estação", "Choose a station", "Elige una emisora"):currentRadio.name,32,220,22,TEXT,Paint.Align.LEFT,true);
        text(c,currentRadio==null?tr("Internet necessária", "Internet required", "Internet necesaria"):currentRadio.country+" • "+currentRadio.codec+" • "+currentRadio.bitrate+" kbps",32,252,14,MUTED,Paint.Align.LEFT,false);
        button(c,new RectF(32,275,16+playerW-16,323),playback!=null&&playback.playing()?"❚❚  PAUSAR":"▶  TOCAR",true);
        int y=(int)listY;
        for(int i=radioStart;i<stations.size() && y<getHeight()-132;i++){
            RadioStation s=stations.get(i); row(c,new RectF(listX,y,listX+listW,y+58),s.name,s.country+" • "+s.codec+" • "+s.bitrate+" kbps","◉"); y+=64;
        }
        if(stations.isEmpty()){ text(c,tr("Carregando estações…", "Loading stations…", "Cargando emisoras…"),listX+12,listY+30,16,MUTED,Paint.Align.LEFT,false); loadRadio(); }
    }

    private void drawNavigation(Canvas c) {
        float y=getHeight()-66,w=getWidth()/4f;
        p.setColor(0xff10151e);c.drawRect(0,y,getWidth(),getHeight(),p);
        String[] n={"⌂\nHOME","♫\nMÚSICA","▶\nVÍDEO","◉\nRÁDIO"};
        for(int i=0;i<4;i++){ text(c,n[i].substring(0,1),w*i+w/2,y+24,20,page==i?ACCENT:MUTED,Paint.Align.CENTER,true); text(c,n[i].substring(2),w*i+w/2,y+47,11,page==i?TEXT:MUTED,Paint.Align.CENTER,true); }
    }

    private void drawTrackInfo(Canvas c,MediaItem m,float x,float y,float maxW){
        if(m==null){text(c,tr("Nenhuma música encontrada", "No music found", "No se encontró música"),x,y,19,TEXT,Paint.Align.LEFT,true);return;}
        text(c,fit(m.title,30),x,y,24,TEXT,Paint.Align.LEFT,true); text(c,fit(m.artist+" — "+m.album,44),x,y+30,15,ACCENT,Paint.Align.LEFT,false);
        String info=ext(m.title)+"  •  "+formatSize(m.size)+"  •  "+formatTime(m.duration); text(c,info,x,y+60,14,MUTED,Paint.Align.LEFT,false);
        text(c,m.mime,x,y+84,13,MUTED,Paint.Align.LEFT,false);
    }

    private void controls(Canvas c,float cx,float y){
        text(c,"‹‹",cx-78,y,32,MUTED,Paint.Align.CENTER,true); circle(c,cx,y-10,30,TEXT); text(c,playback!=null&&playback.playing()?"❚❚":"▶",cx+2,y,23,BG,Paint.Align.CENTER,true); text(c,"››",cx+78,y,32,MUTED,Paint.Align.CENTER,true);
        long len=playback==null?0:playback.length(),time=playback==null?0:playback.time();float left=cx-115; p.setStrokeWidth(6);p.setColor(BORDER);c.drawLine(left,y+38,cx+115,y+38,p); if(len>0){p.setColor(ACCENT);c.drawLine(left,y+38,left+230f*time/len,y+38,p);} text(c,formatTime(time),left,y+62,12,MUTED,Paint.Align.LEFT,false);text(c,formatTime(len),cx+115,y+62,12,MUTED,Paint.Align.RIGHT,false);
    }

    private void drawQueue(Canvas c,float x,float y,float w,float h){
        text(c,tr("A seguir", "Up next", "A continuación"),x,y,17,TEXT,Paint.Align.LEFT,true);int yy=(int)y+15,shown=0;
        for(MediaItem m:media)if(!m.video&&m!=current&&shown<4&&yy+56<getHeight()-70){row(c,new RectF(x,yy,x+w,yy+52),m.title,m.artist,"♫");yy+=58;shown++;}
        if(shown==0)text(c,tr("A fila aparecerá aqui", "The queue will appear here", "La cola aparecerá aquí"),x,y+42,14,MUTED,Paint.Align.LEFT,false);
    }

    @Override public boolean onTouchEvent(MotionEvent e) {
        if(e.getAction()!=MotionEvent.ACTION_UP)return true;float x=e.getX(),y=e.getY(),w=getWidth(),h=getHeight();
        if(showTerms){if(y>h-200){showTerms=false;invalidate();}return true;}
        if(onboarding){
            if(y<60&&x>w-120){showTerms=true;}
            else if(y>h*.51f&&y<h*.60f){language=x<w/2-48?"pt-BR":x>w/2+48?"es":"en";}
            else if(y>h*.62f&&y<h*.71f)acceptedTerms=!acceptedTerms;
            else if(y>h*.78f&&y<h*.90f&&acceptedTerms){onboarding=false;prefs.edit().putBoolean("onboarded",true).putBoolean("terms",true).putString("lang",language).apply();}
            invalidate();return true;
        }
        if(y>h-78){page=Math.min(3,(int)(x/(w/4f)));if(page==3)loadRadio();invalidate();return true;}
        if(page==0){boolean portrait=h>w;if(portrait&&y>500&&folderPicker!=null){folderPicker.run();return true;}int idx=portrait?(int)((y-108)/124):(int)((x-18)/((w-48)/3));if(idx>=0&&idx<3){page=idx+1;if(page==3)loadRadio();invalidate();}}
        else if(page==1){float controlX=h>w?w/2f:150,controlY=h>w?465:385;if(Math.abs(y-controlY)<55){if(x<controlX-38)previous();else if(x>controlX+38)next();else if(playback!=null&&current!=null){if(playback.title().equals(current.title))playback.toggle();else playback.play(current);}}}
        else if(page==2){int idx=(int)((y-105)/66),seen=0;for(MediaItem m:media)if(m.video){if(seen++==idx){current=m;playback.play(m);break;}}}
        else {boolean portrait=h>w;float listY=portrait?355:104,listX=portrait?16:w*.48f;if(y>=listY&&x>=listX){int idx=radioStart+(int)((y-listY)/64);if(idx>=0&&idx<stations.size()){currentRadio=stations.get(idx);playback.playRadio(currentRadio.name,currentRadio.url);}}else if(y>265&&y<335&&currentRadio!=null){playback.toggle();}}
        invalidate();return true;
    }

    private void loadRadio(){if(!stations.isEmpty()||radioLoading)return;radioLoading=true;RadioBrowserClient.search(radioFilter,(list,error)->post(()->{radioLoading=false;stations.clear();stations.addAll(list);invalidate();}));}
    private void next(){int i=current==null?-1:media.indexOf(current);for(int n=1;n<=media.size();n++){MediaItem m=media.get((i+n)%media.size());if(!m.video){current=m;playback.play(m);break;}}}
    private void previous(){int i=current==null?0:media.indexOf(current);for(int n=1;n<=media.size();n++){int q=(i-n+media.size())%media.size();MediaItem m=media.get(q);if(!m.video){current=m;playback.play(m);break;}}}
    private MediaItem first(boolean video){for(MediaItem m:media)if(m.video==video)return m;return null;}
    private int count(boolean video){int n=0;for(MediaItem m:media)if(m.video==video)n++;return n;}
    private void header(Canvas c,String title,String sub){text(c,"K",20,42,31,ACCENT,Paint.Align.LEFT,true);text(c,title,60,35,23,TEXT,Paint.Align.LEFT,true);text(c,sub,60,59,13,MUTED,Paint.Align.LEFT,false);p.setColor(BORDER);c.drawRect(16,78,getWidth()-16,80,p);}
    private void card(Canvas c,RectF r,String title,String sub,String icon){panel(c,r);text(c,icon,r.left+22,r.top+52,36,ACCENT,Paint.Align.LEFT,true);text(c,title,r.left+78,r.top+42,20,TEXT,Paint.Align.LEFT,true);text(c,sub,r.left+78,r.top+70,14,MUTED,Paint.Align.LEFT,false);}
    private void row(Canvas c,RectF r,String title,String sub,String icon){panel(c,r);text(c,icon,r.left+16,r.centerY()+7,19,ACCENT,Paint.Align.LEFT,true);text(c,fit(title,34),r.left+48,r.top+24,16,TEXT,Paint.Align.LEFT,true);text(c,fit(sub,42),r.left+48,r.top+45,12,MUTED,Paint.Align.LEFT,false);}
    private void panel(Canvas c,RectF r){p.setColor(PANEL);c.drawRoundRect(r,14,14,p);p.setStyle(Paint.Style.STROKE);p.setStrokeWidth(1);p.setColor(BORDER);c.drawRoundRect(r,14,14,p);p.setStyle(Paint.Style.FILL);}
    private void cover(Canvas c,RectF r){panel(c,r);circle(c,r.centerX(),r.centerY(),r.width()*.31f,0xff0c1018);circle(c,r.centerX(),r.centerY(),r.width()*.08f,ACCENT);text(c,"K",r.centerX(),r.centerY()+13,35,TEXT,Paint.Align.CENTER,true);}
    private void button(Canvas c,RectF r,String label,boolean enabled){p.setColor(enabled?ACCENT:BORDER);c.drawRoundRect(r,13,13,p);text(c,label,r.centerX(),r.centerY()+6,14,enabled?BG:MUTED,Paint.Align.CENTER,true);}
    private void check(Canvas c,float x,float y,boolean on,String label){RectF r=new RectF(x,y-17,x+28,y+11);p.setColor(on?ACCENT:PANEL);c.drawRoundRect(r,6,6,p);text(c,on?"✓":"",x+14,y+5,17,BG,Paint.Align.CENTER,true);text(c,label,x+40,y+4,14,TEXT,Paint.Align.LEFT,false);}
    private void circle(Canvas c,float x,float y,float radius,int color){p.setColor(color);c.drawCircle(x,y,radius,p);}
    private void empty(Canvas c,String s){text(c,s,getWidth()/2f,getHeight()/2f,18,MUTED,Paint.Align.CENTER,false);}
    private void text(Canvas c,String s,float x,float y,float size,int color,Paint.Align align,boolean bold){p.setColor(color);p.setTextSize(size);p.setTextAlign(align);p.setTypeface(bold?android.graphics.Typeface.DEFAULT_BOLD:android.graphics.Typeface.DEFAULT);c.drawText(s==null?"":s,x,y,p);}
    private String fit(String s,int n){if(s==null)return "";return s.length()>n?s.substring(0,n-1)+"…":s;}
    private String ext(String s){int i=s==null?-1:s.lastIndexOf('.');return i<0?"ÁUDIO":s.substring(i+1).toUpperCase(Locale.ROOT);}
    private String formatSize(long n){if(n<=0)return "-- MB";return String.format(Locale.getDefault(),"%.1f MB",n/1048576f);}
    private String formatTime(long ms){long sec=Math.max(0,ms/1000);return String.format(Locale.getDefault(),"%02d:%02d",sec/60,sec%60);}
}
