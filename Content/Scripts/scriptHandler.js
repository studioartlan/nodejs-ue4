const util = require('util');
const childProcess = require('child_process');
const IPC = require('ipc-event-emitter').default;
const IPCEventEmitter = require('ipc-event-emitter').IPCEventEmitter;

const k = require('./constants.js');
const uf = require('./utilityFunctions.js');

const startScript = (scriptName, socket, scriptPath)=>{	
	//default path is home path
	if(!scriptPath){
		scriptPath = './';
	}
	let fullScriptPath = scriptPath + scriptName;

	let child = childProcess.fork(scriptPath + scriptName, [], { silent: true });
	ipc = new IPCEventEmitter(child);
	let pid = child.pid;

	//emitLog(fullScriptPath + ' started.');

	//catch messages directly and auto-forward the messages to our socket.io pipe
	child.on('message', data =>{
		//emitLog(data);
		if(	data.type == 'ipc-event-emitter' &&
			data.emit &&
			data.emit.length > 0){
			let eventName = data.emit.shift();
			let args = data.emit;
			
			let combinedEventName = uf.routedEventName(child.pid, eventName);

			//scriptLog(socket, 'event: ' + combinedEventName + ", args: " + args);

			if(socket){
				if(args.length == 1){
					socket.emit(combinedEventName, args[0]);
				}
				else{
					socket.emit(combinedEventName, args);
				}
				
			}
		}
	});

	let lastError = "";

	//wrap around stderr to catch compile errors
	child.stderr.setEncoding('utf8');
	child.stderr.on('data', (err) => {
		//for some reason we get newlines separately, just spit out the long error event
		if(err.length>1){
			//set last error, we will emit on process.exit
			lastError = lastError + err.trim();
		}
	});

	//handle console.log from child processes
	child.stdout.setEncoding('utf8');
	child.stdout.on('data', (msg)=>{
		let finalMsg = msg.toString().trim();
		//emitLog('got: ' + msg);
		uf.scriptLog(socket, finalMsg, pid);
	});

	child.on('exit', (code, signal) =>{
		//emitLog(fullScriptPath + ' finished with ' + code);
		//abnormal process exit, emit error back to unreal via sio
		if(code == 1){
			//emitLog('child error: ' + lastError);
			if(socket){
				socket.emit(k.childScriptError, lastError);
				lastError = "";
			}
		}
		
		if(socket){
			socket.emit(k.childScriptEnd, pid);
		}

		//clear up our ipc
		ipc.isRunning = false;
		child = null;
	});

	ipc.isRunning = true;

	let result = {};
	result.ipc = ipc;
	result.child = child;

	return result;
}

const stopScript = (processInfo, callback)=>{
	if(	processInfo &&
		processInfo.ipc &&
		processInfo.child &&
		processInfo.child.connected){
		const processId = processInfo.child.pid;

		//emitLog(util.inspect(processInfo));

		processInfo.ipc.emit('kill');
		setTimeout(()=>{
			try{
				processInfo.child.disconnect();
				callback(null, processId);
			}
			catch(e){
				callback('script disconnect error: ' + util.inspect(e));
			}
		},100);
	}
	else{
		callback(processId + ' process no longer valid for termination.');
	}
}

const startNpmScript = (targetPath, callback) =>{
	/*if(!callback){
		callback = ()=>{};
	}*/

	//startup the npmManager script
	let scriptName = 'npmManager.js';
	let child = childProcess.fork(k.pluginContentScriptsFolder + scriptName, [], { silent: true });
	ipc = new IPCEventEmitter(child);
	let pid = child.pid;

	child.stderr.setEncoding('utf8');
	child.stderr.on('data', (err) => {
		console.log(err);
	});

	console.log('started npm with ' + pid);

	//bind ipc data and emit call
	ipc.on('installIfNeededCallback', (result)=>{
		callback(result);
		ipc.emit('quit');
	});

	ipc.emit('installIfNeeded', targetPath);
}

exports.startScript = startScript;
exports.stopScript = stopScript;
exports.startNpmScript = startNpmScript;