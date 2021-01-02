//
//  HTML PAGE
//

const char PAGE_AdminMainPage[] PROGMEM = R"=====(
<meta name="viewport" content="width=device-width, initial-scale=1" />
<hr>
<strong><div id="city" name="city"></div></strong>
<table border="0" cellspacing="0" cellpadding="3" style="width:300px">
    <tr>
      <td align="right">Current: </td>
      <td><input type="text" id="temp" name="temp" value="" style="width: 80px;" disabled = "true">&degC</td>
    </tr>
	<tr>
      <td align="right">Forecast: </td>
      <td><input type="text" id="forecast" name="forecast" value="" style="width: 80px;" disabled = "true">&degC</td>
    </tr>
    <tr>
      <td align="right">Short Description:</td>
      <td><input type="text" id="weather" name="weather" value="" style="width: 200px;" disabled = "true"></td>
    </tr>
</table>
<hr>
<strong>Configuration</strong><br>
<hr>
<form action="/" method="post">
  <table border="0" cellspacing="0" cellpadding="3" style="width:300px">
    <tr>
      <td align="right">Auto Location</td>
      <td><input type="checkbox" id="autolocation" name="autolocation" value="true" onclick="document.getElementById('latitude').disabled=this.checked;
          document.getElementById('longitude').disabled=this.checked;"></td>
    </tr>
    <tr>
      <td align="right">Latitude</td>
      <td><input type="number" id="latitude" name="latitude" value="" max = "90" min = "-90" style="width: 80px;" step="0.0001"></td>
    </tr>
    <tr>
      <td align="right">Longitude</td>
      <td><input type="number" id="longitude" name="longitude" value="" max = "180" min = "-180" style="width: 80px;" step="0.0001"></td>
    </tr>
    <tr>
      <td colspan="2"><small>Values in the western and southern hemispheres are -ve</small></td>
    </tr>
    <tr><td align="right">Switch On (0-12)</td><td><input type="number" id="switch_on" name="switch_on" value="" min="0" max="12"></td></tr>
    <tr><td align="right">Switch Off (13-23)</td><td><input type="number" id="switch_off" name="switch_off" value="" min="13" max="23"></td></tr>
    <tr><td align="right">Effects (0 to switch off)</td><td><input type="number" id="effects" name="effects" value="" min="0" max="59"></td></tr>
    <tr>
      <td colspan="2" align="center"><input type="submit" style="width:150px" class="btn btn--m btn--blue" value="Save">
      </td>
    </tr>
  </table>
</form>
<hr>
<strong>Administration</strong>
<hr>
<a href="autoupdate" style="width:250px" class="btn btn--m btn--blue" >Check for New Firmware</a><br>
<a href="factory" style="width:250px" class="btn btn--m btn--blue" >Factory Reset</a><br>
<a href="update" style="width:250px" class="btn btn--m btn--blue" >Upload New Firmware</a><br>
<small><div id="displayid" hidden></div>
<p>Designed and created by Dushyant Ahuja (2020)</p>
<p>Weather information provided by openweathermap.org</p>
</small>
<hr>
<script>
window.onload = function ()
{
	load("style.css","css", function() 
	{
		load("microajax.js","js", function() 
		{
				setValues("/admin/config");
		});
	});
}
setTimeout(function () {
    document.getElementById('latitude').disabled = document.getElementById('autolocation').checked;
    document.getElementById('longitude').disabled = document.getElementById('autolocation').checked;
  }, 1000);

window.setInterval(function () {
    setValues("/admin/weather");
  }, 5000);

function load(e,t,n){if("js"==t){var a=document.createElement("script");a.src=e,a.type="text/javascript",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}else if("css"==t){var a=document.createElement("link");a.href=e,a.rel="stylesheet",a.type="text/css",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}}
</script>
)=====";