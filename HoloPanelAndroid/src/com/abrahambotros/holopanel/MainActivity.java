package com.abrahambotros.holopanel;

import java.util.logging.Level;

import org.joda.time.LocalDateTime;
import org.joda.time.format.DateTimeFormat;
import org.joda.time.format.DateTimeFormatter;
import org.json.JSONException;
import org.json.JSONObject;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewFrame;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewListener2;
import org.opencv.android.JavaCameraView;
import org.opencv.core.Mat;

import retrofit.Callback;
import retrofit.RetrofitError;
import retrofit.client.Response;
import retrofit.mime.TypedByteArray;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.common.api.GoogleApiClient.ConnectionCallbacks;
import com.google.android.gms.common.api.GoogleApiClient.OnConnectionFailedListener;
import com.google.android.gms.location.LocationServices;

import android.R.integer;
import android.R.string;
import android.app.Activity;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.graphics.Point;
import android.location.Geocoder;
import android.location.Location;
import android.os.BatteryManager;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.ResultReceiver;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.view.View.OnTouchListener;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;


public class MainActivity extends Activity implements CvCameraViewListener2, OnTouchListener, ConnectionCallbacks, OnConnectionFailedListener {
	
	// native
	private long mNativeController = 0;
	
	// camera
	private JavaCameraView mCameraView;
	private boolean receivedInitialTouch = false;
	private boolean requiresInit = false;
	
	// panel
	private final int PANEL_LABEL_NONE = 0,
			PANEL_LABEL_TOP_LEFT_RECT = 1,
			PANEL_LABEL_TOP_CENTER_RECT = 2,
			PANEL_LABEL_TOP_RIGHT_RECT = 3,
			PANEL_LABEL_BOTTOM_LEFT_CIRCLE = 4,
			PANEL_LABEL_BOTTOM_RIGHT_CIRCLE = 5;
	private final int PANEL_ACTION_PICTURE = 1,
			PANEL_ACTION_WEATHER = 2,
			PANEL_ACTION_SYSTEMINFO = 3,
			PANEL_ACTION_EMAIL = 4,
			PANEL_ACTION_SOCIALMEDIA = 5;
	private final int PANEL_ICON_SELECTED_BACKGROUND_COLOR = 0x5500fbff;
	private int previousPanelSelection = PANEL_LABEL_NONE,
			newPanelSelection = PANEL_LABEL_NONE;

	// retrofit api client
	private ApiService mApiService;

	// google api client and location
	private GoogleApiClient mGoogleApiClient;
	private Location mLastLocation;
	private AddressResultReceiver mResultReceiver;
	private String mAddressOutput;

	// UI
	private ImageView pictureViewIcon, weatherViewIcon, systemInfoViewIcon, emailViewIcon, socialMediaViewIcon;
	private LinearLayout pictureViewContainer, weatherViewContainer, systemInfoViewContainer, emailViewContainer, socialMediaViewContainer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // prepare camera
        prepareCamera();
        
        // prepare UI/get UI references
        prepareUI();
        
        // prepare location stuff
        prepareLocationStuff();
        
        // prepare ApiClient (RetroFit)
        prepareApiClient();
        
        // prepare Google Play Services API
        prepareGoogleApiClient();
    }
    
    private void prepareCamera() {
    	// check screen resolution
    	try {
    		DisplayMetrics displayMetrics = new DisplayMetrics();
    		getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
    		Integer heightPixels = displayMetrics.heightPixels;
    		Integer widthPixels = displayMetrics.widthPixels;
    		if (heightPixels != 1104 || widthPixels != 1920) {
    			Toast.makeText(this, "Unsupported screen resolution detected. Usable screen area expected: (1920, 1104). Got: ("
    					+ widthPixels.toString() + ", " + heightPixels.toString() + "). Expect suboptimal behavior.", Toast.LENGTH_LONG).show();
    		}
    	} catch (Exception e) {
    		e.printStackTrace();
    	}

    	// prepare camera view
    	setContentView(R.layout.activity_main);
    	mCameraView = (JavaCameraView) findViewById(R.id.camera_view);
    	mCameraView.setVisibility(SurfaceView.VISIBLE);
    	mCameraView.setCvCameraViewListener(this);
    	mCameraView.setMaxFrameSize(720, 480);
    	mCameraView.enableFpsMeter();
    	
    	// always-on screen
    	getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }
    
    private void prepareUI() {
    	
    	// get UI references (icons)
    	pictureViewIcon = (ImageView) findViewById(R.id.picture_view_icon);
    	weatherViewIcon = (ImageView) findViewById(R.id.weather_view_icon);
    	systemInfoViewIcon = (ImageView) findViewById(R.id.systemInfo_view_icon);
    	emailViewIcon = (ImageView) findViewById(R.id.email_view_icon);
    	socialMediaViewIcon = (ImageView) findViewById(R.id.socialMedia_view_icon);

    	// get UI references (containers)
    	pictureViewContainer = (LinearLayout) findViewById(R.id.picture_view_container);
    	weatherViewContainer = (LinearLayout) findViewById(R.id.weather_view_container);
    	systemInfoViewContainer = (LinearLayout) findViewById(R.id.systemInfo_view_container);
    	emailViewContainer = (LinearLayout) findViewById(R.id.email_view_container);
    	socialMediaViewContainer = (LinearLayout) findViewById(R.id.socialMedia_view_container);
    	
    }
    
    // prepare location stuff
    private void prepareLocationStuff() {
    	mResultReceiver = new AddressResultReceiver(new Handler());
    }
    
    // RetroFit API client
    private void prepareApiClient() {
    	mApiService = ApiServiceSingleton.getInstance();
    }
    
    // Google API client
    private synchronized void prepareGoogleApiClient() {
    	mGoogleApiClient = new GoogleApiClient.Builder(this)
    			.addConnectionCallbacks(this)
    			.addOnConnectionFailedListener(this)
    			.addApi(LocationServices.API)
    			.build();
    	mGoogleApiClient.connect();
    }

    // Google API client
	@Override
	public void onConnectionFailed(ConnectionResult result) {
		Toast.makeText(this, "Unable to connect to Google Play Services. Location and weather unavailable.", Toast.LENGTH_LONG).show();
	}

    // Google API client
	@Override
	public void onConnected(Bundle connectionHint) {
		
		// get last location
		mLastLocation = getLastLocation();
		
		// handle last location
		if (mLastLocation != null) {
			
			// log
			Log.i("HoloPanel", "Got lastLocation = (" + String.valueOf(mLastLocation.getLatitude()) + ", " + String.valueOf(mLastLocation.getLongitude()) + ")");
			
			// make sure Geocoder is present
			if (!Geocoder.isPresent()) {
				Log.i("HoloPanel", "Geocoder unavailable. Not starting FetchAddressIntentService.");
			} else { 
				// create fetchAddressIntentService using this location
				startFetchAddressIntentService();
			}
			
		} else {
			Log.i("HoloPanel", "Unable to get last known location.");
		}
		
	}

    // Google API client
	@Override
	public void onConnectionSuspended(int cause) {
		Toast.makeText(this, "Lost connection to Google Play Services. Location and weather possibly unavailable.", Toast.LENGTH_LONG).show();
	}
	
	// Google API client + Location
	private Location getLastLocation() { 
		return LocationServices.FusedLocationApi.getLastLocation(mGoogleApiClient);
	}
	
	// start FetchAddressIntentService using mLastLocation
	private void startFetchAddressIntentService() {
		Intent intent = new Intent(this, FetchAddressIntentService.class);
		intent.putExtra(FetchAddressIntentService.RECEIVER, mResultReceiver);
		intent.putExtra(FetchAddressIntentService.LOCATION_DATA_EXTRA, mLastLocation);
		startService(intent);
	}
	
	// AddressResultReceiver class
	class AddressResultReceiver extends ResultReceiver {
		public AddressResultReceiver(Handler handler) {
			super(handler);
		}
		
		@Override
		protected void onReceiveResult(int resultCode, Bundle resultData) {
			// Display address string or error message from intent service
			mAddressOutput = resultData.getString(FetchAddressIntentService.RESULT_DATA_KEY);
			// if successfully found address
			if (resultCode == FetchAddressIntentService.SUCCESS_RESULT) {
				// log
				Log.i("HoloPanel", "Found address. mAddressOutput=" + mAddressOutput);
				// send address to Yahoo Weather API and update weather UI
				getWeatherFromAddress();
			} else {
				Log.i("HoloPanel", "MainActivity.onReceiveResult: failure result. mAddressOutput/errorMessage=" + mAddressOutput);
			}
			
		}
	}
	
	// get weather from mAddressOutput address string using RetroFit and Yahoo Weather API
	private void getWeatherFromAddress() {
		
		// convert mAddressOutput to queryString
		String queryString = "select item.condition from weather.forecast where woeid in (select woeid from geo.places(1) where text=\"" + mAddressOutput + "\")";
		
		// make api call
		mApiService.getWeatherFromAddress(queryString, "json", new Callback<WeatherObject>() {
			
			@Override
			public void success(final WeatherObject weatherObject, Response arg1) {
				
				// log
				Log.i("MainActivity:getWeatherFromAddress", "Retrofit success callback");
				
				// update UI using weatherObject
				updateWeatherUI(weatherObject);
				
			}
			
			@Override
			public void failure(RetrofitError error) {
				Log.i("MainActivity:getWeatherFromAddress", "RetrofitError. " + new String(((TypedByteArray) error.getResponse().getBody()).getBytes()));
			}
		});
	}
	
	// update weather UI with weatherObject
	// - use codes at
	private void updateWeatherUI(WeatherObject weatherObject) {
		
		// parse weatherObject
		String conditionString = weatherObject.getConditionString();
		Integer conditionCode = weatherObject.getConditionCode();
		String tempString = weatherObject.getTempInteger().toString();
		
		// get UI references
		TextView weatherViewTextView = (TextView) findViewById(R.id.weather_view_text);
		ImageView weatherViewImageView = (ImageView) findViewById(R.id.weather_view_picture);
		
		// set text
		weatherViewTextView.setText(tempString + "\u00B0 - " + conditionString + "\n" + mAddressOutput); // TODO: UNCOMMENT!!!
		//weatherViewTextView.setText(tempString + "\u00B0 - " + conditionString + "\nSan Francisco, CA 94122"); // TODO: REMOVE!!!
		
		// set image
		
		// cloudy
		if (conditionCode == 25 || conditionCode == 26) {
			weatherViewImageView.setImageResource(R.drawable.weather_cloudy);
		}
		// drizzle
		else if (conditionCode == 8 || conditionCode == 9) {
			weatherViewImageView.setImageResource(R.drawable.weather_drizzle);
		}
		// haze
		else if (19 <= conditionCode && conditionCode <= 24) {
			weatherViewImageView.setImageResource(R.drawable.weather_haze);
		}
		// mostly cloudy
		else if ( (27 <= conditionCode && conditionCode <= 30) || (conditionCode == 44)) {
			weatherViewImageView.setImageResource(R.drawable.weather_mostly_cloudy);
		}
		// slight drizzle - unused
		// snow
		else if ( (41 <= conditionCode && conditionCode <= 43) || (5 <= conditionCode && conditionCode <= 7) || (13 <= conditionCode && conditionCode <= 18) || (conditionCode == 46)) {
			weatherViewImageView.setImageResource(R.drawable.weather_snow);
		}
		// thunderstorms
		else if ( (conditionCode == 3) || (37 <= conditionCode && conditionCode <= 39) || (conditionCode == 45) || (conditionCode == 47) ) {
			weatherViewImageView.setImageResource(R.drawable.weather_thunderstorms);
		}
		// sunny
		else if ( (31 <= conditionCode && conditionCode <= 34) || conditionCode == 36 ) {
			weatherViewImageView.setImageResource(R.drawable.weather_sunny);
		}
		// else
		else {
			weatherViewImageView.setImageResource(R.drawable.ic_more_horiz_white_24dp);
		}
	}
	
    @Override
    public void onResume() {
    	super.onResume();
    	
    	// create native controller
    	mNativeController = createNativeController();
    	
    	// enable camera
    	mCameraView.enableView();
    	
    	// set touch listener
    	mCameraView.setOnTouchListener(MainActivity.this);
    	
    	// toast for initialization
    	Toast.makeText(this, "Place hand in box and touch screen to calibrate", Toast.LENGTH_LONG).show();
    	
    	//// log
    	//Log.d("HoloPanelMainActivity", Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES).toString());
    	//Log.i("MainActivity.java", "mCameraView size: height=" + ((Integer) mCameraView.getHeight()).toString() + ", width=" + ((Integer) mCameraView.getWidth()).toString());
    }
    
    @Override
    public void onPause() {
    	super.onPause();
    	
    	// disable camera
    	if (mCameraView != null) {
    		mCameraView.disableView();
    	}
    	
    	// destroy native controller
    	if (mNativeController != 0) {
    		destroyNativeController(mNativeController);
    	}
    		
    }
    
    @Override
    public void onDestroy() {
    	super.onDestroy();
    	
    	// disable camera
    	if (mCameraView != null) {
    		mCameraView.disableView();
    	}
    	
    }
   
	@Override
	public boolean onTouch(View v, MotionEvent event) {
		Log.i("MainActivity.java event", ((Float) event.getX()).toString() + ", " + ((Float)event.getY()).toString());
		Log.i("MainActivity.java view", ((Float) v.getX()).toString() + ", " + ((Float)v.getY()).toString());
		
		// flip receivedInitialTouch on touch; allows triggering: pre -> calibrate -> run
		receivedInitialTouch = !receivedInitialTouch;
		//// any time have at least one touch, set receivedInitialTouch to true
		//receivedInitialTouch = true;
		// set requiresInit
		requiresInit = true;
		// toast initializing
		Toast.makeText(this, "Initializing", Toast.LENGTH_SHORT).show();
		// return
		return false;
	}

	@Override
	public void onCameraViewStarted(int width, int height) {
		Toast.makeText(this, "onCameraViewStarted()", Toast.LENGTH_SHORT).show();
	}


	@Override
	public void onCameraViewStopped() {
		Toast.makeText(this, "onCameraViewStopped()", Toast.LENGTH_SHORT).show();
	}


	@Override
	public Mat onCameraFrame(CvCameraViewFrame inputFrame) {
		
		// get rgba frame
		Mat frame = inputFrame.rgba();
		
		// handle frame, passing in receivedInitialTouch state and requiresInit state
		final int panelSelection = handleFrame(mNativeController, frame.getNativeObjAddr(), receivedInitialTouch, requiresInit);
		
		// if received initial touch, then reset requiresInit
		if (receivedInitialTouch) {
			requiresInit = false;
		}

		// if new panelSelection, send panelSelection on separate thread to main activity
		if (panelSelection != previousPanelSelection) {
			runOnUiThread(new Runnable() {
				@Override
				public void run() {
					handleNewPanelSelection(panelSelection);
				}
			});
		}

		// return
		return frame;
	}
	
	// trigger panel selection action in Android
	// - assumes this is a new panel selection as evaluated in onCameraFrame
	private void handleNewPanelSelection(final int panelSelection) {
		// log
		Log.i("HoloPanel", String.valueOf(panelSelection));
		
		// set new panel selection
		newPanelSelection = panelSelection;
		
		// clear previous panel selection if not PANEL_LABEL_NONE
		clearPreviousPanelSelection();
		
		// execute new panel selection
		displayNewPanelSelection();
		
		// set previous to current/new
		previousPanelSelection = newPanelSelection;
	}
	
	// clear old panel selection on screen
	private void clearPreviousPanelSelection() {
		
		// reset background for all icons
		pictureViewIcon.setBackgroundColor(Color.TRANSPARENT);
		weatherViewIcon.setBackgroundColor(Color.TRANSPARENT);
		systemInfoViewIcon.setBackgroundColor(Color.TRANSPARENT);
		emailViewIcon.setBackgroundColor(Color.TRANSPARENT);
		socialMediaViewIcon.setBackgroundColor(Color.TRANSPARENT);
		
		// hide each view
		// TODO: make array of views, call setVisibility(GONE) on each
		pictureViewContainer.setVisibility(View.GONE);
		weatherViewContainer.setVisibility(View.GONE);
		systemInfoViewContainer.setVisibility(View.GONE);
		emailViewContainer.setVisibility(View.GONE);
		socialMediaViewContainer.setVisibility(View.GONE);
		
	}
	
	// show new panel selection
	private void displayNewPanelSelection() {
		
		switch (newPanelSelection) {
		case PANEL_ACTION_PICTURE:
			showPicture();
			break;
		case PANEL_ACTION_WEATHER:
			showWeather();
			break;
		case PANEL_ACTION_SYSTEMINFO:
			showSystemInfo();
			break;
		case PANEL_ACTION_EMAIL:
			showEmail();
			break;
		case PANEL_ACTION_SOCIALMEDIA:
			showSocialMedia();
			break;
		default:
			break;
		}
	}
	
	// show picture view
	private void showPicture() {
		// add background to icon
		pictureViewIcon.setBackgroundColor(PANEL_ICON_SELECTED_BACKGROUND_COLOR);

		// make container visible
		pictureViewContainer.setVisibility(View.VISIBLE);
	}

	// show weather view
	private void showWeather() {
		
		/*
		// get last known location if not already found
		if (mLastLocation == null) {
			mLastLocation = getLastLocation();
		}
		*/
		// get UI refernces
		TextView weatherViewTextView = (TextView) findViewById(R.id.weather_view_text); 

		// if still null, then show fail text // TODO: update weather textView to say (weather unavailable) or something
		if (mLastLocation == null) {
			Toast.makeText(this, "Unable to get location. Weather unavailable.", Toast.LENGTH_SHORT).show();
			weatherViewTextView.setText("Unable to get location. Weather unavailable.");
		} else {
			
			// send location latitude and longitude to Yahoo Weather API using Retrofit
		}
		// TODO: implement successful location -> weather api -> show weather

		// add background to icon
		weatherViewIcon.setBackgroundColor(PANEL_ICON_SELECTED_BACKGROUND_COLOR);

		// make container visible
		weatherViewContainer.setVisibility(View.VISIBLE);
	}

	// show time view
	private void showSystemInfo() {
		
		// get time string
		LocalDateTime localDateTime = LocalDateTime.now();
		DateTimeFormatter dateTimeFormatter = DateTimeFormat.forPattern("hh:mm a\nE, MMM dd");
		String localDateTimeString = localDateTime.toString(dateTimeFormatter);
		
		// get battery level
		IntentFilter intentFilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED); // TODO: constant
		Intent batteryStatus = this.registerReceiver(null, intentFilter);
		int batteryLevel = batteryStatus.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
		int batteryScale = batteryStatus.getIntExtra(BatteryManager.EXTRA_SCALE, -1);
		//float batteryPercent = batteryLevel / (float) batteryScale;
		String batteryPercentString = String.valueOf((int) (100 * batteryLevel / (float) batteryScale)) + "%";
		
		// show time and battery level string in UI
		TextView systemInfoTimeTextView = (TextView) findViewById(R.id.systemInfoTime_view_text);
		TextView systemInfoBatteryTextView = (TextView) findViewById(R.id.systemInfoBattery_view_text);
		//String viewText = localDateTimeString + "\n\nBattery: " + batteryPercentString;
		systemInfoTimeTextView.setText(localDateTimeString);
		systemInfoBatteryTextView.setText(batteryPercentString);

		// add background to icon
		systemInfoViewIcon.setBackgroundColor(PANEL_ICON_SELECTED_BACKGROUND_COLOR);

		// make container visible
		systemInfoViewContainer.setVisibility(View.VISIBLE);
	}

	// show email view
	private void showEmail() {
		// add background to icon
		emailViewIcon.setBackgroundColor(PANEL_ICON_SELECTED_BACKGROUND_COLOR);

		// make container visible
		emailViewContainer.setVisibility(View.VISIBLE);
	}

	// show social media view
	private void showSocialMedia() {
		// add background to icon
		socialMediaViewIcon.setBackgroundColor(PANEL_ICON_SELECTED_BACKGROUND_COLOR);

		// make container visible
		socialMediaViewContainer.setVisibility(View.VISIBLE);
	}

	
	// native
	public native long createNativeController();
	public native void destroyNativeController(long mNativeController_addr);
	//public native void initialize(long mNativeController_addr, long frameRgba_addr);
	public native int handleFrame(long mNativeController_addr, long frameRgba_addr, boolean receivedInitialTouch, boolean requires_init);

	// library
    static {
    	System.loadLibrary("HoloPanel");
    }

}
