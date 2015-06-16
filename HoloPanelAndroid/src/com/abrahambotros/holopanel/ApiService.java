package com.abrahambotros.holopanel;

import retrofit.Callback;
import retrofit.client.Response;
import retrofit.http.GET;
import retrofit.http.Query;

public interface ApiService {
	
	@GET("/v1/public/yql")
	//void getWeatherFromAddress(final String addressString, Callback<String> cb);
	void getWeatherFromAddress(@Query("q") String queryString, @Query("format") String formatString, Callback<WeatherObject> cb);


}
