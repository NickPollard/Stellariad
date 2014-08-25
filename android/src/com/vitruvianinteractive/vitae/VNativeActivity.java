package com.vitruvianinteractive.vitae;

import android.app.Activity;
import android.app.NativeActivity;
import android.os.Bundle;
import android.util.Log;
import android.content.pm.*;
import android.content.pm.PackageManager.NameNotFoundException;
import android.view.View;
import android.view.View.*;
import android.app.ActionBar;

public class VNativeActivity extends NativeActivity
{
	// Native function declaration
	public native void setApkPath( String path );

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {	
		// Call C to setup APK path
		String apkFilePath = null;
		ApplicationInfo appInfo = null;
		PackageManager packMgmr = this.getPackageManager();
		try {
			appInfo = packMgmr.getApplicationInfo("com.vitruvianinteractive.vitae", 0);
		} catch (NameNotFoundException e) {
			e.printStackTrace();
			throw new RuntimeException("Unable to locate assets, aborting...");
		}
		apkFilePath = appInfo.sourceDir;
        System.loadLibrary( "vitae" );
		setApkPath( apkFilePath );

		hideStatusBar();

        super.onCreate(savedInstanceState);
	}

	public void onResume() {
		hideStatusBar();

		super.onResume();
	}

	// TODO Android 4.1 and above - 4.0 or below requires different code
	void hideStatusBar() {
		int uiOptions = View.SYSTEM_UI_FLAG_FULLSCREEN
		 				| View.SYSTEM_UI_FLAG_IMMERSIVE
		 				| View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
		 				| View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
		getWindow().getDecorView().setSystemUiVisibility(uiOptions);
	}
}
