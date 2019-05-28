
var express = require('express');
var app = express();
var expressWs = require('express-ws')(app);
const bodyParser = require('body-parser');

app.set('view engine', 'ejs');

app.use(bodyParser.urlencoded({ extended: false }));
app.use(bodyParser.json());

let state = 0.00;

app.get('/', function (req, res) {
    res.render('pages/index');
    //res.sendFile(__dirname + '/index.html');
});

app.get('/current', function (req, res) {
    return res.send(`${state}`);
});


app.post('/data', (req, res) => {
    var data = req.body.data;
    state = data;
    console.log("Got " + data + " from ESP8266");
    res.send("OK\n");
});






var server = app.listen(8081, () => {
    var host = server.address().address;
    var port = server.address().port;

    console.log("ZAP Server listening at http://%s:%s", host, port)
})



/*
var app = require('express')();
var http = require('http').Server(app);
var io = require('socket.io')(http);
var port = process.env.PORT || 3000;

app.get('/', function(req, res){
  res.sendFile(__dirname + '/index.html');
});

io.on('connection', function(socket){
  socket.on('chat message', function(msg){
    io.emit('chat message', msg);
  });
});

http.listen(port, function(){
  console.log('listening on *:' + port);
});
*/
