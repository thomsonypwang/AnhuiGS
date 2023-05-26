#ifndef _WEBPAGE_H_
#define _WEBPAGE_H_

/*************************************************************************************
 * JavaScript Functions
 *************************************************************************************/
#define w5500_ajax_js  "function AJAX(url, callback)"\
						"{"\
							"var req = AJAX_init();"\
							"req.onreadystatechange = AJAX_processRequest;"\
							"function AJAX_init()"\
							"{"\
								"if (window.XMLHttpRequest)"\
								"{"\
									"return new XMLHttpRequest();"\
								"}"\
								"else if (window.ActiveXObject)"\
								"{"\
									"return new ActiveXObject('Microsoft.XMLHTTP');"\
								"}"\
							"};"\
							"function AJAX_processRequest()"\
							"{"\
								"if(req.readyState == 4)"\
								"{"\
									"if(req.status == 200)"\
									"{"\
										"if(callback)"\
											"callback(req.responseText);"\
									"}"\
								"}"\
							"};"\
							"this.doGet = function()"\
							"{"\
								"req.open('GET', url, true);"\
								"req.send(null);"\
							"};"\
							"this.doPost = function(body)"\
							"{"\
								"req.open('POST', url, true);"\
								"req.setRequestHeader('Content-Type','application/x-www-form-urlencoded');"\
								"req.setRequestHeader('ISAJAX','yes');"\
								"req.send(body);"\
							"};"\
						"};"\
						"function $(id)"\
						"{"\
							"return document.getElementById(id);"\
						"}"\
						"function $$(id)"\
						"{"\
							"return document.getElementsByName(id);"\
						"}"\
						"function $$_ie(tag, name)"\
						"{"\
							"if(!tag)"\
							"{"\
								"tag='*';"\
							"}"\
							"var elems=document.getElementsByTagName(tag);"\
							"var res=[];"\
							"for(var i=0;i<elems.length;i++)"\
							"{"\
								"att=elems[i].getAttribute('name');"\
								"if(att==name)"\
								"{"\
									"res.push(elems[i]);"\
								"}"\
							"}"\
							"return res;"\
						"}"\
						"function selset(id,val)"\
						"{"\
							"var o=$(id);"\
							"for(var i=0;i<o.options.length;i++)"\
							"{"\
								"if(i==val)"\
								"{"\
									"o.options[i].selected=true;"\
									"break;"\
								"}"\
							"}"\
						"}"\
						"function selset_name(name, val)"\
						"{"\
							"var o=$$(name);"\
							"for(var i=0;i<o.options.length;i++)"\
							"{"\
								"if(i==val)"\
								"{"\
									"o.options[i].selected=true;"\
									"break;"\
								"}"\
							"}"\
						"}"


#define w5500_info_js  	"var dhcp_enabled = 0;"\
							"function NetinfoCallback(o)"\
							"{"\
								"$('txtPcode').value=o.pcode;"\
								"$('txtFwver').value=o.fwver;"\
								"$('txtDname').value=o.devname;"\
								"$('txtMac').value=o.mac;"\
								"$('txtIp').value=o.ip;"\
								"$('txtGw').value=o.gw;"\
								"$('txtSub').value=o.sub;"\
								"$('txtDns').value=o.dns;"\
								"$('mqttuse').value=o.use;"\
								"$('mqttpass').value=o.pass;"\
								"$('uptime').value=o.time;"\
								"$('mqttip').value=o.mip;"\
								"$('mqttport').value=o.mport;"\
								"$('severip1').value=o.sip1;"\
								"$('severport1').value=o.sport1;"\
								"$('severip2').value=o.sip2;"\
								"$('severport2').value=o.sport2;"\
								"$('severip3').value=o.sip3;"\
								"$('severport3').value=o.sport3;"\
								"$('mqttpop').value=o.mpop;"\
								"$('mqttsub').value=o.msub;"\
								"$('mqttpopa').value=o.mpopa;"\
								"$('mqttpopc').value=o.mpopc;"\
								"$('mqttpopb').value=o.mpopb;"\
								"$('tempfs').value=o.fan1;"\
								"$('tempfc').value=o.fan2;"\
								"$('humihs').value=o.fhot1;"\
								"$('humihc').value=o.fhot2;"\
								"if(typeof(window.external)!='undefined')"\
								"{"\
									"obj=$$_ie('input','dhcp');"\
								"}"\
								"else"\
								"{"\
									"obj=$$('dhcp');"\
								"}"\
								"for(var i=0; i<obj.length;i++)"\
								"{"\
									"if(i==o.dhcp)"\
									"{"\
										"obj[i].checked=true;"\
										"netinfo_block(i);"\
										"break;"\
									"}"\
								"}"\
							"}"\
							"function getinfo()"\
							"{"\
								"var oUpdate;"\
								"setTimeout(function(){oUpdate=new AJAX('get_info.cgi',function(t){try{eval(t);}catch(e){alert(e);}});oUpdate.doGet();},300);"\
							"}"\
							"function netinfo_block(o)"\
							"{"\
								"if(o == 1)"\
								"{"\
									"$('txtIp').disabled=true;"\
									"$('txtGw').disabled=true;"\
									"$('txtSub').disabled=true;"\
									"$('txtDns').disabled=true;"\
									"dhcp_enabled = 1;"\
								"}"\
								"else"\
								"{"\
									"$('txtIp').disabled=false;"\
									"$('txtGw').disabled=false;"\
									"$('txtSub').disabled=false;"\
									"$('txtDns').disabled=false;"\
									"dhcp_enabled = 0;"\
								"}"\
							"}"
							
							
//#define w5500_setNetinfo_js \
//							"function setNetinfo(o)"\
//							"{"\
//								"var param = o.attributes['param'].value;"\
//								"var str;"\
//								"var set_enable = 1;"\
//								"if(dhcp_enabled == 0)"\
//								"{"\
//									"if(param == 'ip'){str = $('txtIp').value;}"\
//									"else if(param == 'gw'){str = $('txtGw').value;}"\
//									"else if(param == 'sub'){str = $('txtSub').value;}"\
//									"else if(param == 'dns'){str = $('txtDns').value;}"\
//									"else if(param == 'dhcp'){str = 0;}"\
//									"else {set_enable = 0;}"\
//								"}"\
//								"else"\
//								"{"\
//									"if(param == 'dhcp'){str = 1;}"\
//									"else {set_enable = 0;}"\
//								"}"\
//								"if(set_enable != 0)"\
//								"{"\
//									"setTimeout(function()"\
//									"{"\
//										 "dout=new AJAX('set_netinfo.cgi', function(t){try{eval(t);}catch(e){alert(e);}});"\
//										 "dout.doPost(param+'='+str);"\
//									"},300);"\
//								"}"\
//							"}"

//#define w5500_setMqttinfo_js \
//							"function setMqttinfo(o)"\
//							"{"\
//								"var param = o.attributes['param'].value;"\
//								"var str;"\
//								"if(param == 'use'){str = $('mqttuse').value;}"\
//								"else if(param == 'pass'){str = $('mqttpass').value;}"\
//								"else if(param == 'time'){str = $('uptime').value;}"\
//								"else if(param == 'mip'){str = $('mqttip').value;}"\
//								"else if(param == 'mport'){str = $('mqttport').value;}"\
//								"else if(param == 'sip1'){str = $('severip1').value;}"\
//								"else if(param == 'sport1'){str = $('severport1').value;}"\
//								"else if(param == 'sip2'){str = $('severip2').value;}"\
//								"else if(param == 'sport2'){str = $('severport2').value;}"\
//								"else if(param == 'sip3'){str = $('severip3').value;}"\
//								"else if(param == 'sport3'){str = $('severport3').value;}"\
//								"else if(param == 'mpop'){str = $('mqttpop').value;}"\
//								"else if(param == 'msub'){str = $('mqttsub').value;}"\
//								"else if(param == 'mpopa'){str = $('mqttpopa').value;}"\
//								"else if(param == 'mpopc'){str = $('mqttpopc').value;}"\
//								"else if(param == 'mpopb'){str = $('mqttpopb').value;}"\
//								"setTimeout(function()"\
//								"{"\
//									"dout=new AJAX('set_mqttinfo.cgi', function(t){try{eval(t);}catch(e){alert(e);}});"\
//									"dout.doPost(param+'='+str);"\
//								"},300);"\
//							"}"

//#define w5500_setthinfo_js \
//							"function setthinfo(o)"\
//							"{"\
//								"var param = o.attributes['param'].value;"\
//								"var str;"\
//								"if(param == 'fan1'){str = $('tempfs').value;}"\
//								"else if(param == 'fan2'){str = $('tempfc').value;}"\
//								"else if(param == 'fhot1'){str = $('humihs').value;}"\
//								"else if(param == 'fhot2'){str = $('humihc').value;}"\
//								"setTimeout(function()"\
//								"{"\
//									"dout=new AJAX('set_thinfo.cgi', function(t){try{eval(t);}catch(e){alert(e);}});"\
//									"dout.doPost(param+'='+str);"\
//								"},300);"\
//							"}"

//	"<script type='text/javascript' src='setMqttinfo.js'></script>"\
//	"<script type='text/javascript' src='setthinfo.js'></script>"\
//"<script type='text/javascript' src='setNetinfo.js'></script>"\

#define w5500_setinfo_js \
							"function setinfo(o)"\
							"{"\
								"var param = o.attributes['param'].value;"\
								"var str;"\
								"if(param == 'use'){str = $('mqttuse').value;}"\
								"else if(param == 'pass'){str = $('mqttpass').value;}"\
								"else if(param == 'time'){str = $('uptime').value;}"\
								"else if(param == 'mip'){str = $('mqttip').value;}"\
								"else if(param == 'mport'){str = $('mqttport').value;}"\
								"else if(param == 'sip1'){str = $('severip1').value;}"\
								"else if(param == 'sport1'){str = $('severport1').value;}"\
								"else if(param == 'sip2'){str = $('severip2').value;}"\
								"else if(param == 'sport2'){str = $('severport2').value;}"\
								"else if(param == 'sip3'){str = $('severip3').value;}"\
								"else if(param == 'sport3'){str = $('severport3').value;}"\
								"else if(param == 'mpop'){str = $('mqttpop').value;}"\
								"else if(param == 'msub'){str = $('mqttsub').value;}"\
								"else if(param == 'mpopa'){str = $('mqttpopa').value;}"\
								"else if(param == 'mpopc'){str = $('mqttpopc').value;}"\
								"else if(param == 'mpopb'){str = $('mqttpopb').value;}"\
								"else if(param == 'fan1'){str = $('tempfs').value;}"\
								"else if(param == 'fan2'){str = $('tempfc').value;}"\
								"else if(param == 'fhot1'){str = $('humihs').value;}"\
								"else if(param == 'fhot2'){str = $('humihc').value;}"\
								"else if(param == 'ip'){str = $('txtIp').value;}"\
								"else if(param == 'gw'){str = $('txtGw').value;}"\
								"else if(param == 'sub'){str = $('txtSub').value;}"\
								"else if(param == 'dns'){str = $('txtDns').value;}"\
								"else if(param == 'save'){str = 1;}"\
								"setTimeout(function()"\
								"{"\
									"dout=new AJAX('set_info.cgi', function(t){try{eval(t);}catch(e){alert(e);}});"\
									"dout.doPost(param+'='+str);"\
								"},300);"\
							"}"

//favicon.ico							
#define pageico "\0" 

/*************************************************************************************
 * HTML Pages (web pages)
 *************************************************************************************/

#define index_page "<!DOCTYPE html>"\
	"<html>"\
	"<head>"\
	"<title>Gateway web</title>"\
	"<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />"\
	"<meta http-equiv='pragma' content='no-cache' />"\
	"<meta http-equiv='content-type' content='no-cache, must-revalidate' />"\
	"<style>"\
		"body{background-color:transparent;}"\
		"body,h3,p,div{margin:0;padding:0;font: normal 14px 'omnes-pro', Helvetica, Arial, sans-serif;}"\
		"#header"\
		"{"\
			"position: relative;"\
			"margin: auto;"\
		"}"\
		"#header a"\
		"{"\
			"margin-left: 30px;"\
		"}"\
		"#header h1"\
		"{"\
			"vertical-align: middle;"\
			"font-size: 42px;"\
			"font-weight: bold;"\
			"text-decoration: none;"\
			"color: #000;"\
			"margin-left: 30px;"\
			"text-align: center;"\
		"}"\
		"#header h2"\
		"{"\
			"vertical-align: middle;"\
			"font-size: 30px;"\
			"font-weight: bold;"\
			"text-decoration: none;"\
			"color: #000;"\
			"margin-left: 30px;"\
			"text-align: left;"\
		"}"\
		".usual"\
		"{"\
			"background:transparent;"\
			"color:#111;"\
			"padding:15px 20px;"\
			"width:auto;"\
			"margin:8px auto;"\
		"}"\
		".usual li {list-style:none; float:left;}"\
		".usual ul a"\
		"{"\
			"display:block;"\
			"padding:6px 10px;"\
			"text-decoration:none!important;"\
			"margin:1px;"\
			"margin-left:0;"\
			"font-weight:bold;"\
			"color:#FFF;"\
			"background:#aaa;"\
		"}"\
		".usual div"\
		"{"\
			"padding: 10px 10px 10px 10px;"\
			"*padding-top:3px;"\
			"*margin-top:15px;"\
			"clear:left;"\
			"background:#EEF3FF;"\
		"}"\
		".ipt label{float:left;padding-top:3px;text-align:left;width:130px;}"\
		".usual h3"\
		"{"\
			"margin-top:10px;"\
			"margin-left:10px;"\
			"font-size:24px;"\
			"text-decoration:none;"\
			"font-weight:bold;"\
			"color:blue;"\
		"}"\
		".usual p{margin-top:0;margin-bottom:10px;}"\
		".usual label{margin-left:10px;}"\
		".red{color:red}"\
		".analog{margin-top:2px;margin-right:10px;border:1px solid #ccc;height:20px;width:500px;display:block;}"\
		".ain{width:0%;height:100%;text-align:center;background:red;float:left;display:block;}"\
		".info div{margin:0 auto;text-align:left;display:table-cell;vertical-align:middle;background:#FFE4E1;}"\
		".btn "\
		"{"\
			"background: #3498db;"\
			"background-image: -webkit-linear-gradient(top, #3498db, #2980b9);"\
			"background-image: -moz-linear-gradient(top, #3498db, #2980b9);"\
			"background-image: -ms-linear-gradient(top, #3498db, #2980b9);"\
			"background-image: -o-linear-gradient(top, #3498db, #2980b9);"\
			"background-image: linear-gradient(to bottom, #3498db, #2980b9);"\
			"-webkit-border-radius: 28;"\
			"-moz-border-radius: 28;"\
			"border-radius: 28px;"\
			"color: #ffffff;"\
			"font-size: 13px;"\
			"padding: 3px 10px 3px 10px;"\
			"text-decoration: none;"\
		"}"\
		".btn:hover"\
		"{"\
			"background: #3cb0fd;"\
			"background-image: -webkit-linear-gradient(top, #3cb0fd, #3498db);"\
			"background-image: -moz-linear-gradient(top, #3cb0fd, #3498db);"\
			"background-image: -ms-linear-gradient(top, #3cb0fd, #3498db);"\
			"background-image: -o-linear-gradient(top, #3cb0fd, #3498db);"\
			"background-image: linear-gradient(to bottom, #3cb0fd, #3498db);"\
			"cursor: pointer;"\
			"text-decoration: none;"\
		"}"\
	"</style>"\
	"<script type='text/javascript' src='ajax.js'></script>"\
	"<script type='text/javascript' src='info.js'></script>"\
	"<script type='text/javascript' src='setinfo.js'></script>"\
	"</head>"\
	"<body onload='getinfo();'>"\
		"<div id='header'>"\
			"<h1>网关板配置</h1>"\
		"</div>"\
		"<div class='usual'>"\
			"<h3>设备信息</h3>"\
			"<br>"\
				"<div  class='ipt'>"\
					"<p><label for='txtPcode'>产品名称: </label><input type='text' id='txtPcode' size='20' disabled='disabled' value='' /></p>"\
					"<p><label for='txtFwver'>软件版本:</label><input type='text' id='txtFwver' size='20' disabled='disabled' value='' /></p>"\
					"<p><label for='txtDname'>设备ID:</label><input type='text' id='txtDname' size='32' disabled='disabled' value='' /></p>"\
				"</div>"\
			"<br>"\
			"<h3>设备网络配置</h3>"\
			"<br>"\
				"<div>"\
					"<input type='radio' name='dhcp' id='rdStatic' value='0' disabled='disabled' /><label for='rdStatic'>静态IP</label>"\
					"<input type='radio' name='dhcp' id='rdDhcp' value='1' disabled='disabled' /><label for='rdDhcp'>DHCP</label>"\
				"</div>"\
				"<div class='ipt'>"\
					"<p><label for='txtMac'>MAC地址: </label><input type='text' id='txtMac' size='20' disabled='disabled' value='' /></p>"\
					"<p><label for='txtIp'>设备IP地址:</label><input id='txtIp' name='ip' type='text' size='20' value=''/> <input type='button' class='btn' value='保存' param='ip' onclick='setinfo(this);'></p>"\
					"<p><label for='txtGw'>网关地址:</label><input id='txtGw' name='gw' type='text' size='20' value=''/> <input type='button' class='btn' value='保存' param='gw' onclick='setinfo(this);'></p>"\
					"<p><label for='txtSub'>子网掩码:</label><input id='txtSub' name='sub' type='text' size='20' value=''/> <input type='button' class='btn' id='set_sub' param='sub' value='保存' onclick='setinfo(this);'></p>"\
					"<p><label for='txtDns'>DNS服务器:</label><input id='txtDns' name='dns' type='text' size='20' value=''/> <input type='button' class='btn' id='set_dns' param='dns' value='保存' onclick='setinfo(this);'></p>"\
				"</div>"\
			"<br>"\
			"<h3>MQTT服务器配置</h3>"\
			"<br>"\
				"<div class='ipt'>"\
					"<p><label for='mqttuse'>MQTT登录账号:</label><input id='mqttuse' name='use' type='text' size='32' value=''/> <input type='button' class='btn' value='保存' param='use' onclick='setinfo(this);'></p>"\
					"<p><label for='mqttpass'>MQTT登录密码:</label><input id='mqttpass' name='pass' type='text' size='32' value=''/> <input type='button' class='btn' value='保存' param='pass' onclick='setinfo(this);'></p>"\
					"<p><label for='uptime'>MQTT推送时间(分):</label><input id='uptime' name='time' type='text' size='5' value=''/> <input type='button' class='btn' value='保存' param='time' onclick='setinfo(this);'></p>"\
					"<p><label for='mqttip'>MQTT服务器IP:</label><input id='mqttip' name='mip' type='text' size='20' value=''/> <input type='button' class='btn' value='保存' param='mip' onclick='setinfo(this);'></p>"\
					"<p><label for='mqttport'>MQTT服务器端口号:</label><input id='mqttport' name='mport' type='text' size='5' value=''/><input type='button' class='btn' value='保存' param='mport' onclick='setinfo(this);'></p>"\
					"<p><label for='severip1'>服务器1IP:</label><input id='severip1' name='sip1' type='text' size='20' value=''/><input type='button' class='btn' value='保存' param='sip1' onclick='setinfo(this);'></p>"\
					"<p><label for='severport1'>服务器1端口号:</label><input id='severport1' name='sport1' type='text' size='5' value=''/><input type='button' class='btn' value='保存' param='sport1' onclick='setinfo(this);'></p>"\
					"<p><label for='severip2'>服务器2IP:</label><input id='severip2' name='sip2' type='text' size='20' value=''/><input type='button' class='btn' value='保存' param='sip2' onclick='setinfo(this);'></p>"\
					"<p><label for='severport2'>服务器2端口号:</label><input id='severport2' name='sport2' type='text' size='5' value=''/><input type='button' class='btn' value='保存' param='sport2' onclick='setinfo(this);'></p>"\
					"<p><label for='severip3'>服务器3IP:</label><input id='severip3' name='sip3' type='text' size='20' value=''/><input type='button' class='btn' value='保存' param='sip3' onclick='setinfo(this);'></p>"\
					"<p><label for='severport3'>服务器3端口号:</label><input id='severport3' name='sport3' type='text' size='5' value=''/><input type='button' class='btn' value='保存' param='sport3' onclick='setinfo(this);'></p>"\
					"<p><label for='mqttpop'>MQTT推送主题:</label><input id='mqttpop' name='mpop' type='text' size='64' value=''/> <input type='button' class='btn' value='保存' param='mpop' onclick='setinfo(this);'></p>"\
					"<p><label for='mqttsub'>MQTT订阅主题:</label><input id='mqttsub' name='msub' type='text' size='64' value=''/> <input type='button' class='btn' value='保存' param='msub' onclick='setinfo(this);'></p>"\
					"<p><label for='mqttpopa'>MQTT报警主题:</label><input id='mqttpopa' name='mpopa' type='text' size='64' value=''/> <input type='button' class='btn' value='保存' param='mpopa' onclick='setinfo(this);'></p>"\
					"<p><label for='mqttpopc'>MQTT推送回复主题:</label><input id='mqttpopc' name='mpopc' type='text' size='64' value=''/> <input type='button' class='btn' value='保存' param='mpopc' onclick='setinfo(this);'></p>"\
					"<p><label for='mqttpopb'>MQTT推送保留主题:</label><input id='mqttpopb' name='mpopb' type='text' size='64' value=''/> <input type='button' class='btn' value='保存' param='mpopb' onclick='setinfo(this);'></p>"\
				"</div>"\
			"<br>"\
			"<h3>温湿度设定</h3>"\
			"<br>"\
				"<div class='ipt'>"\
					"<p><label for='tempfs'>高温启动风扇值:</label><input id='tempfs' name='fan1' type='number' min='30' max='100' step='1' size='4' value=''/> <input type='button' class='btn' value='保存' param='fan1' onclick='setinfo(this);'></p>"\
					"<p><label for='tempfc'>高温关闭风扇值:</label><input id='tempfc' name='fan2' type='number' min='30' max='100' step='1'size='4' value=''/> <input type='button' class='btn' value='保存' param='fan2' onclick='setinfo(this);'></p>"\
					"<p><label for='humihs'>高湿启动风扇加热值:</label><input id='humihs' name='fhot1' type='number' min='0' max='100' step='1'size='4' value=''/> <input type='button' class='btn' value='保存' param='fhot1' onclick='setinfo(this);'></p>"\
					"<p><label for='humihc'>高温关闭风扇加热值:</label><input id='humihc' name='fhot2' type='number' min='0' max='100' step='1' size='4' value=''/> <input type='button' class='btn' value='保存' param='fhot2' onclick='setinfo(this);'></p>"\
				"</div>"\
			"<br>"\
			"<p><center><input type='button' class='btn' value='重启' param='save' onclick='setinfo(this);'></center></p>"\
		"</div>"\
		"<div style='margin:5px 5px; clear:both' >"\
			"<center>"\
				"&copy;Copyright 2023 Kiloamp Co., Ltd."\
			"</center>"\
		"</div>"\
	"</body>"\
	"</html>"
	
#endif
