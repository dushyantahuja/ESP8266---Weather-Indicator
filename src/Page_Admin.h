//
//  HTML PAGE
//

const char PAGE_AdminMainPage[] PROGMEM = R"=====(
<meta name="viewport" content="width=device-width, initial-scale=1" />
<strong>Administration</strong>
<hr>
<a href="config.html" style="width:250px" class="btn btn--m btn--blue" >Weather Configuration</a><br>
<a href="update" style="width:250px" class="btn btn--m btn--blue" >Upload New Firmware</a><br>
<hr>
<strong><input type="text" id="city" name="city" style="width: 200px" disabled = "true"></strong>
<table border="0" cellspacing="0" cellpadding="3" style="width:300px">
    <tr>
      <td align="right">Latitude: </td>
      <td><input type="number" id="latitude" name="latitude" value="" style="width: 80px;" disabled = "true"></td>
    </tr>
    <tr>
      <td align="right">Longitude: </td>
      <td><input type="text" id="longitude" name="longitude" value="" style="width: 80px;" disabled = "true"></td>
    </tr>
    <tr>
      <td align="right">Current: </td>
      <td><input type="text" id="temp" name="temp" value="" style="width: 80px;" disabled = "true">&degC</td>
    </tr>
	<tr>
      <td align="right">Forecast: </td>
      <td><input type="text" id="forecast" name="forecast" value="" style="width: 80px;" disabled = "true">&degC</td>
    </tr>
    <tr>
      <td align="right">Weather:</td>
      <td><input type="text" id="weather" name="weather" value="" style="width: 200px;" disabled = "true"></td>
    </tr>
  </table>
<script>
window.onload = function ()
{
	load("style.css","css", function() 
	{
		load("microajax.js","js", function() 
		{
				setValues("/admin/weather");
		});
	});
}

window.setInterval(function () {
    setValues("/admin/weather");
  }, 5000);

function load(e,t,n){if("js"==t){var a=document.createElement("script");a.src=e,a.type="text/javascript",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}else if("css"==t){var a=document.createElement("link");a.href=e,a.rel="stylesheet",a.type="text/css",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}}
</script>
)=====";