package com.droidlogic.SubTitleService;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.os.ServiceManager;
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
                if (ServiceManager.getService("subtitle_service") == null) {
                    subtitleService = new SubTitleService (context);
                    ServiceManager.addService (/*Context.SUBTITLE_SERVICE*/"subtitle_service", subtitleService);
                } else {
                    Log.i (TAG, "subtitle_service is already added.");
                }
            }
        }
}

