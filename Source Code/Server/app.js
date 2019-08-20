/*
app.js สำหรับสร้าง Protocol IoT device.
code.tommy.com
coder : tommy
created : 2019.Aug.19
#V.100
*/

const express = require('express');
const app = express();
var port = 4000;

const moment = require('moment');
var Promise = require('promise');

var mongojs = require('mongojs');
var dhtdb = mongojs('myiotdb');
var mycoll = 'lora01';

var data, datasize, dataset='';
var t, h;

var timediff = 28800;

// อันนี้คือ root ถ้ายิงเข้ามาจาก postman จะเจอฟังก์ชั่นนี้ก่อน
app.get('/', function (req, res) {
  console.log("my protocol already service");
  res.send("my iot Protocol ready !");
});

// นี้คือฟังก์ชั่นรับค่าเอาไปเขียนลงใน Mongo อีกที
app.get('/write/:data', function (req, res) {
  var strParseWriteReq = JSON.stringify(req.params);
  var strWriteReq = JSON.parse(strParseWriteReq);
  data = strWriteReq.data;
  writedata(data, res);
});

app.get('/read/:datasize', function (req, res) {
  var strParseReadReq = JSON.stringify(req.params);
  var strReadReq = JSON.parse(strParseReadReq);
  datasize = strReadReq.datasize;
  readdata(datasize, res);
});

async function readdata(_datasize, res){
  await readDataFromMongo(_datasize, res);
}

function readDataFromMongo(_readdatasize, res){
  return new Promise(function(resolve,reject){
    var dhtcollection = dhtdb.collection(mycoll);
    dhtcollection.find({}).limit(Number(_readdatasize)).sort({recordTime: -1}, function(err, docs){
      console.log(JSON.stringify(docs));
      res.jsonp(docs);
    });
  });
}

/* For DHT write */
app.get('/writedht/:t/:h', function (req, res) {
  var strParseWriteReq = JSON.stringify(req.params);
  var strWriteReq = JSON.parse(strParseWriteReq);
  t = strWriteReq.t;
  h = strWriteReq.h;
  console.log(t + ":" + h);
  writeDHT(t, h, res);
});

/* For DHT data read */
app.get('/readdht/:datasize', function (req, res) {
  var strParseReadReq = JSON.stringify(req.params);
  var strReadReq = JSON.parse(strParseReadReq);
  datasize = strReadReq.datasize;
  readDHT(datasize, res);
});

app.listen(port, function () {
  var nodeStartTime = new Date();
  console.log('My IoT protocol running on port ' + port + ' start at ' + nodeStartTime);
});

async function writeDHT(_t, _h, res){
  await writeDHTtoMongo(_t, _h, res);
}

function writeDHTtoMongo(_saveT, _saveH, res){
  return new Promise(function(resolve, reject){
    d = new Date();
    ts = parseInt(new Date().getTime()/1000, 10) + timediff;     
    ts_frmt = moment(ts * 1000).format('DD/MM/YYYY HH:mm');

    var dhtwritecollection = dhtdb.collection(mycoll);
    dhtwritecollection.insert({
      t: Number(_saveT),
      h: Number(_saveH),
      rssi: 0,
      recordTime: ts_frmt,
      ts: ts
    }, function(err){
      if(err){
        console.log(err);
        res.send(String(err));
      }else {
        console.log('record dht data ok');
        res.send('record dht data ok');
      }
    });
  });
}

async function readDHT(_datasize, res){
  await readDHTFromMongo(_datasize, res);
}

function readDHTFromMongo(_readdatasize, res){
  return new Promise(function(resolve,reject){
    var dhtcollection = dhtdb.collection(mycoll);
    dhtcollection.find({}).limit(Number(_readdatasize)).sort({recordTime: -1}, function(err, docs){
      console.log(JSON.stringify(docs));
      res.jsonp(docs);
    });
  });
}
