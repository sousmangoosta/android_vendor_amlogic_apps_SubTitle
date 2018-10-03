package com.droidlogic.SubTitleService;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
//import android.os.ServiceManager;
import android.os.IBinder;

import java.lang.reflect.Method;
import com.droidlogic.app.SystemControlManager;

public class SubTitleServiceBroadcastReceiver extends BroadcastReceiver {
        private static final String TAG = "SubTitleServiceBroadcastReceiver";
        private SubTitleService subtitleService = null;
        private SystemControlManager mSystemControl;

        @Override
        public void onReceive (Context context, Intent intent) {
            String action = intent.getAction();
            Log.i (TAG, "[onReceive]action:" + action + ", subtitleService:" + subtitleService);

            mSystemControl = new SystemControlManager(context);
            if (mSystemControl.getPropertyBoolean("sys.subtitle.disable", true)) {
                return;
            }

            if (Intent.ACTION_BOOT_COMPLETED.equals (action)) {
                if (getSubtitleService() == null) {
                    subtitleService = new SubTitleService (context);
                    addSubtitleService();
                    //ServiceManager.addService (/*Context.SUBTITLE_SERVICE*/"subtitle_service", subtitleService);
                } else {
                    Log.i (TAG, "subtitle_service is already added.");
                }
            }
        }


        public boolean addSubtitleService() {
            try {
                Class<?> sm = Class.forName("android.os.ServiceManager");
                Method addSubtitleService = sm.getMethod("addService", String.class,IBinder.class);
                addSubtitleService.invoke(null, "subtitle_service");
                return true;
            } catch (Exception e) {
                e.printStackTrace();
                return false;
            }
        }

        public IBinder getSubtitleService() {
            try {
                Class<?> sm = Class.forName("android.os.ServiceManager");
                Method getSubtitleService = sm.getMethod("getService", String.class);
                IBinder mSubtitleService = (IBinder)getSubtitleService.invoke(null, "subtitle_service");
                return mSubtitleService;
            } catch (Exception e) {
                e.printStackTrace();
                return null;
            }
        }
}

