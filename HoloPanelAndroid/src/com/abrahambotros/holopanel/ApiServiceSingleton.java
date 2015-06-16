package com.abrahambotros.holopanel;

import android.util.Log;
import retrofit.RestAdapter;

public class ApiServiceSingleton {
	
	private static RestAdapter restAdapter;
	private static final ApiService INSTANCE = createInstance();
	
	// base url
	private static final String BASE_URL = "https://query.yahooapis.com";
	
	// create instance
	private static ApiService createInstance() {
		
		// create rest adapter
		restAdapter = new RestAdapter.Builder()
				.setEndpoint(BASE_URL)
				.setLogLevel(RestAdapter.LogLevel.FULL)
				.setLog(new RestAdapter.Log() {
					
					@Override
					public void log(String message) {
						Log.i("restAdapter: ", message);
						
					}
				})
				.build();
		
		// return
		return restAdapter.create(ApiService.class);
	}
	
	// get instance
	public static ApiService getInstance() {
		return INSTANCE;
	}

}
