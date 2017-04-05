package com.subtitleview;

import android.content.Context;
import android.view.View;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.Paint;
import android.graphics.Color;
import android.util.AttributeSet;
import java.lang.Exception;
import android.util.Log;
import android.os.SystemProperties;

public class CcSubView extends View {
    private static final String TAG = "CcSubView";
    private static Bitmap bitmap = null;
    private static final int BUFFER_W = 1920;
    private static final int BUFFER_H = 1080;
    private static int buffer_w;
    private static int buffer_h;
    private static boolean   visible;
    private static Object lock = new Object();
    private int displeft = 0;
    private int dispright = 0;
    private int disptop = 0;
    private int dispbottom = 0;
    private static CcSubView activeView = null;
    private boolean active = true;
    private Context mContext;

    private native int native_cc_init();
    private native int native_cc_destroy();
    private native int native_cc_lock();
    private native int native_cc_unlock();
    private native int native_cc_clear();
    private native int native_start_atsc_cc(int caption, int fg_color, int fg_opacity, int bg_color, int bg_opacity, int font_style, int font_size);
    private native int native_stop_atsc_cc();
    private native int native_cc_set_active(boolean active);

    static{
        System.loadLibrary("am_adp");
        System.loadLibrary("am_mw");
        System.loadLibrary("zvbi");
        System.loadLibrary("ccsubjni");
    }

    private void init(){
        if (bitmap == null) {
            bitmap = Bitmap.createBitmap(BUFFER_W, BUFFER_H, Bitmap.Config.ARGB_8888);
        }
        native_cc_init();
        Log.d(TAG, "native init");
    }

    static public class DTVCCParams{
        private int caption_mode = 0;
        private int fg_color = 0;
        private int fg_opacity = 0;
        private int bg_color = 0;
        private int bg_opacity = 0;
        private int font_style = 0;
        private int font_size = 40;

        public DTVCCParams(int caption, int fg_color, int fg_opacity,
            int bg_color, int bg_opacity, int font_style, int font_size){
            this.caption_mode = caption;
            this.fg_color = fg_color;
            this.fg_opacity = fg_opacity;
            this.bg_color = bg_color;
            this.bg_opacity = bg_opacity;
            this.font_style = font_style;
            this.font_size = font_size;
        }
    }

    DTVCCParams  dtv_cc = new DTVCCParams(0, 0, 0, 0, 1, 0, 0);

    private void update(int w, int h) {
         buffer_w = w;
         buffer_h = h;
         postInvalidate();
    }

    private void update() {
        postInvalidate();
    }

    public void show(){
        if (visible)
            return;

        Log.d(TAG, "show");

        visible = true;
        update();
    }

    public void hide(){
        if (!visible)
            return;

        Log.d(TAG, "hide");

        visible = false;
        update();
    }

    public void startCC(){
        synchronized(lock){

        native_start_atsc_cc(
            dtv_cc.caption_mode,
            dtv_cc.fg_color,
            dtv_cc.fg_opacity,
            dtv_cc.bg_color,
            dtv_cc.bg_opacity,
            dtv_cc.font_style,
            dtv_cc.font_size);

        }

    }

    public void stopCC(){
        synchronized(lock){
            native_stop_atsc_cc();
        }
    }

    public void onDraw(Canvas canvas){
        synchronized(lock){
            Rect sr;
            Rect dr;
            int h_s, h_e, buffer_w_dr, buffer_h_dr;

            buffer_w_dr = Integer.parseInt(SystemProperties.get("sys.subtitleService.ccsize", "0")) * buffer_w / 4 + buffer_w;
            buffer_h_dr = Integer.parseInt(SystemProperties.get("sys.subtitleService.ccsize", "0")) * buffer_h / 4 + buffer_h;

            h_s = getHeight() - buffer_h_dr -Integer.parseInt(SystemProperties.get("sys.subtitleService.cchs", "0")) * buffer_h_dr / 5 / 2;
            h_e = h_s + buffer_h_dr;
            dr = new Rect((getWidth()-buffer_w_dr) /2, h_s, (getWidth()-buffer_w_dr) /2+buffer_w_dr, h_e);

            //Log.d(TAG, "-----h_s: " + h_s + "   h_e: " + h_e + " ******buffer_w: " + buffer_w + "  buffer_h: " + buffer_h + " buffer_w_dr: " + buffer_w_dr + "  buffer_h_dr: " + buffer_h_dr +
            //        " ----getWidth: "+getWidth()+"  getHeight: "+getHeight());

            if (!visible) {
                return;
            }

            native_cc_lock();

            //Log.d(TAG, " CcSubView onDraw");
            sr = new Rect(0, 0, buffer_w, buffer_h);

            canvas.drawBitmap(bitmap, sr, dr, new Paint());

            /*******************************************/
            /*
            Paint paint = new Paint();
            paint.setColor(Color.RED);
            paint.setStyle(Paint.Style.STROKE);
            canvas.drawRect(new Rect(10, 10, 100, 30), paint);

            paint.setColor(Color.GREEN);
            canvas.drawText("1111111111111111111111", 10, 50, paint);
            */
            /********************************************/
            native_cc_unlock();
        }
    }

    public CcSubView(Context context){
        super(context);
        mContext = context;
        init();
    }

    public CcSubView(Context context, AttributeSet attrs){
        super(context, attrs);
        mContext = context;
        init();
    }

    public CcSubView(Context context, AttributeSet attrs, int defStyle){
        super(context, attrs, defStyle);
        init();
    }

    public void setActive(boolean active){
        synchronized(lock){
            native_cc_set_active(active);
            this.active = active;
            if (active) {
                activeView = this;
            /*}else if(activeView == this){
                  activeView = null;*/
            }
            postInvalidate();
        }
    }

    protected void finalize() throws Throwable {
        native_cc_clear();
        native_cc_destroy();
        super.finalize();
    }
}
