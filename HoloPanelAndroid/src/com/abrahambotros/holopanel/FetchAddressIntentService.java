package com.abrahambotros.holopanel;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import android.app.IntentService;
import android.content.Intent;
import android.location.Address;
import android.location.Geocoder;
import android.location.Location;
import android.os.Bundle;
import android.os.ResultReceiver;
import android.text.TextUtils;
import android.util.Log;

// see http://developer.android.com/training/location/display-address.html
public class FetchAddressIntentService extends IntentService {
	
	// constants
	public static final int SUCCESS_RESULT = 0;
	public static final int FAILURE_RESULT = 1;
	public static final String PACKAGE_NAME = "com.abrahambotros.holopanel";
	public static final String RECEIVER = PACKAGE_NAME + ".RECEIVER";
	public static final String RESULT_DATA_KEY = PACKAGE_NAME + ".RESULT_DATA_KEY";
	public static final String LOCATION_DATA_EXTRA = PACKAGE_NAME + ".LOCATION_DATA_EXTRA";
	
	// logging
	private static final String TAG = "FetchAddressIntentService";
	
	// result receiver
	protected ResultReceiver mResultReceiver;

	public FetchAddressIntentService() {
		super("FetchAddressIntentService");
	}

	@Override
	protected void onHandleIntent(Intent intent) {
		
		// get geocoder
		Geocoder geocoder = new Geocoder(this, Locale.getDefault());
		
		// get mResultReceiver passed to this service
		mResultReceiver = intent.getParcelableExtra(RECEIVER);

		// get location passed to this service
		Location location = intent.getParcelableExtra(LOCATION_DATA_EXTRA);
		
		// get addresses
		List<Address> addresses = null;
		String errorMessage = "";
		try {
	        addresses = geocoder.getFromLocation( location.getLatitude(), location.getLongitude(), 1); // In this sample, get just a single address.
	    } catch (IOException ioException) {
	        // Catch network or other I/O problems.
	        errorMessage = "Unable to get address from location - network or other I/O problems";
	        Log.e(TAG, errorMessage, ioException);
	    } catch (IllegalArgumentException illegalArgumentException) {
	        // Catch invalid latitude or longitude values.
	        errorMessage = "Invalid latitude or longitude values.";
	        Log.e(TAG, errorMessage + ". " + "Latitude = " + location.getLatitude() + ", Longitude = " + location.getLongitude(), illegalArgumentException);
	    }

	    // Handle case where no address was found.
	    if (addresses == null || addresses.size()  == 0) {
	        if (errorMessage.isEmpty()) {
	            errorMessage = "Unable to find an address for this location.";
	            Log.e(TAG, errorMessage);
	        }
	        deliverResultToReceiver(FAILURE_RESULT, errorMessage);
	    } else {
	        Address address = addresses.get(0);
	        ArrayList<String> addressFragments = new ArrayList<String>();

	        // Fetch the address lines using getAddressLine,
	        // join them, and send them to the thread.
	        for(int i = 0; i < address.getMaxAddressLineIndex(); i++) {
	            addressFragments.add(address.getAddressLine(i));
	        }
	        Log.i(TAG, "Address found!");
	        deliverResultToReceiver(SUCCESS_RESULT, TextUtils.join(System.getProperty("line.separator"), addressFragments));
	    }
	}
	
	private void deliverResultToReceiver(int resultCode, String message) {
		Bundle bundle = new Bundle();
		bundle.putString(RESULT_DATA_KEY, message);
		mResultReceiver.send(resultCode, bundle);
	}

}
