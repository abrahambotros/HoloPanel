package com.abrahambotros.holopanel;

public class WeatherObject {
	
	// variables
	private WeatherObjectQuery query;
	
	// WeatherObjectQuery
	private class WeatherObjectQuery {
		private WeatherObjectResults results;
	}
	
	// WeatherObjectResults
	private class WeatherObjectResults {
		private WeatherObjectChannel channel;
	}
	
	// WeatherObjectChannel
	private class WeatherObjectChannel {
		private WeatherObjectResultsItem item;
	}
	
	// WeatherObjectResultsItem
	private class WeatherObjectResultsItem {
		private WeatherObjectCondition condition;
	}
	
	// WeatherObjectCondition
	private class WeatherObjectCondition {
		private String text;
		private Integer code;
		private Integer temp;
	}
	
	// functions
	public String getConditionString() {
		try {
			return query.results.channel.item.condition.text;
		} catch (Exception e) {
			e.printStackTrace();
			return "(Condition)";
		}
	}
	
	public Integer getConditionCode() {
		try {
			return query.results.channel.item.condition.code;
		} catch (Exception e) {
			e.printStackTrace();
			return -1;
		}
	}
	
	public Integer getTempInteger() {
		try {
			return query.results.channel.item.condition.temp;
		} catch (Exception e) {
			e.printStackTrace();
			return -1;
		}
	}
	

}
