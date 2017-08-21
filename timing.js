const dgram = require('dgram');
const NS_PER_SEC = 1e9;

let socket = dgram.createSocket('udp4');
socket.bind(45455);

let prevTime = 0;
let startTime = getUS();
let prevIndex = 0;

function getUS()
{
	let pt = process.hrtime();
	return Math.floor((pt[0] * NS_PER_SEC + pt[1]) / 1000000);
}

socket.on('listening', () => {

});

let timeMap = {};

socket.on('message', (message, remote) => {
	let t = getUS();
	//let d = t - prevTime;
	//prevTime = t;
	let d = t - startTime;
	
	/*
	if (remote.address == '192.168.1.106') {
		prevIndex = parseInt(message);
	} else {
		let indexD = parseInt(message) - prevIndex;
		console.log(indexD);
	}
	*/

	timeMap[remote.address] = parseInt(message);

	//console.log(d + ' ' + message + ' ' + remote.address);
});

setInterval(() => {

	console.log('-------------------------------------------');

	let zeroTime = -1;

	for (var key in timeMap) {		

		let t = timeMap[key];

		if (zeroTime == -1) {
			zeroTime = t;
		}

  		console.log(key, t - zeroTime);
	}

}, 500);