print(arguments.length);

var map = new Object();

var fname = arguments[0];
var str = readFile(fname);
var foo = new XML(str);
print(foo.children().length());
print(foo.exe);
print(foo..frame.(sym.toString() == "(null)").length());
var ct = 0;
for each(a in foo..frame) {
    var o = {}
    o.output = "";
    if(a.sym.toString() == "(null)") {
	var res;
	if(!map.hasOwnProperty(a.addr.toString())) {
	    runCommand("addr2line","-C","-f","-e",foo.exe.toString(),a.addr.toString(),o);
	    var s = o.output;
	    s = s.substr(0,s.indexOf("\n"));
	    map[a.addr.toString()] = s;
	    //print(s);
	    //print(ct++);
	}
	a.sym = map[a.addr.toString()];
    }
}

print(foo);

