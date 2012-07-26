K.import("jansson");
K.import("curl");
K.import("logpool");
K.import("konoha");

class DTask {
	int taskid;
	String type;
	int context;
	String method;
	String logpool;
	String script;
	Json jdata;

	DTask(int taskid, String type, int context, String method, String logpool, String script) {
		this.taskid = taskid;
		this.type = type;
		this.context = context;
		this.method = method;
		this.logpool = logpool;
		this.script = script;
		this.jdata = new Json();
		this.jdata.setString("taskid", this.taskid + ""); // TODO setInt();
		this.jdata.setString("type", this.type);
		this.jdata.setString("context", this.context + ""); // TODO setInt();
		this.jdata.setString("method", this.method);
		this.jdata.setString("logpool", this.logpool);
		this.jdata.setString("script", this.script);
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
		String req;
		String res;
		int i = 0;
		while(i < this.tasks.getSize()) {
			req = this.tasks[i].jdata.dump();
			c = new Curl();
			c.setOpt(CURLOPT_URL, url);
			c.appendHeader("Content-Type: application/json");
			c.setOpt(CURLOPT_POSTFIELDS, req);
			c.setOpt(CURLOPT_WRITEDATA, res);
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
