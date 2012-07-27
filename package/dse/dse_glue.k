K.import("jansson");
K.import("curl");
K.import("logpool");
K.import("konoha");
K.import("konoha.bytes");
K.import("posix.file");
K.import("posix.process");

class DTask {
	int taskid;
	String type;
	int context;
	String method;
	String logpool;
	String script;

	DTask(int taskid, String type, int context, String method, String logpool, String script) {
		this.taskid = taskid;
		this.type = type;
		this.context = context;
		this.method = method;
		this.logpool = logpool;
		this.script = script;
	}

	void readFile(String filepath) {
		FILE f = System.fopen(filepath, "r");
		int bufsize = 32;
		Bytes buf = new Bytes(bufsize);
		int ret = bufsize;
		String str = "";
		while(ret == bufsize) {
			buf.setAll(0); // zero clear
			ret = f.read(buf, 0, bufsize);
			if (ret < bufsize) {
				String tmp = buf.toString();
				str = str + tmp;
			} else {
				str = str + buf.toString();
			}
		}
		this.script = str;
	}
}

class DSE {
	String host;
	int port;
	String url;
	DTask[] tasks;
	String[] locks;

	DSE(String host, int port) {
		this.host = host;
		this.port = port;
		this.url = "http://" + host + ":" + port;
		this.tasks = new DTask[0];
		this.locks = new String[0];
	}

	void add(DTask task, boolean isLock) {
		this.tasks.add(task);
		if(isLock) {
			this.locks.add(task.context + "");
		}
	}

	void apply() {
		Curl c;
		Json jdata;
		String req;
		String res;
		int i = 0;
		while(i < this.tasks.getSize()) {
			jdata = new Json();
			jdata.setString("taskid", this.tasks[i].taskid + ""); // TODO setInt();
			jdata.setString("type", this.tasks[i].type);
			jdata.setString("context", this.tasks[i].context + ""); // TODO setInt();
			jdata.setString("method", this.tasks[i].method);
			jdata.setString("logpool", this.tasks[i].logpool);
			jdata.setString("script", this.tasks[i].script);
			req = jdata.dump();
			c = new Curl();
			c.setOpt(CURLOPT_URL, url);
			c.appendHeader("Content-Type: application/json");
			c.setOpt(CURLOPT_POSTFIELDS, req);
			c.setOpt(CURLOPT_WRITEDATA, res);
			p(System.getTime());
			c.perform();
			i += 1;
		}
		this.tasks = new DTask[0];
		this.locks = new String[0];
	}
}

class DSEController {
	void apply(DSE[] dses) {
		int i = 0;
		while(i < dses.getSize()) {
			dses[i].apply();
			i += 1;
		}
	}

	void waitDTasks(String[] locks) { // @Hidden
		int taskSize = locks.getSize();
		Boolean[] isFinished = new Boolean[taskSize];
		int i = 0;
		while(i < taskSize) {
			isFinished[i] = false;
			i += 1;
		}

		int total = 0;
		LogPool lp = new LogPool("0.0.0.0", 14801);
		lp.loadFile("dump", "/usr/local/konoha2/package/dse/query.k");
		Log log;
		while(true) {
			if(total == taskSize) break;
			log = lp.get();
			p(log);
			if(log.isNull()) break;
			if(log.get("TraceID") != "dtask") continue;
			i = 0;
			while(i < taskSize) {
				if(isFinished[i]) {
					i += 1;
					continue;
				}
				if(log.get("context") == locks[i] && log.get("status") == "done") {
					isFinished[i] = true;
					total += 1;
				}
				i += 1;
			}
		}
	}

	void sync(DSE[] dses) {
		int i = 0;
		String[] locks;
		while(i < dses.getSize()) {
			locks = dses[i].locks;
			dses[i].apply();
			waitDTasks(locks);
			i += 1;
		}
	}
}
