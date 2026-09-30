/* LeveretOS guide, interactive modules.
   Every page loads this one file and calls the modules it needs.
   No build step, no dependencies, plain browser JS. */

(function () {
  "use strict";

  /* ============================================================
     helpers
     ============================================================ */

  function el(tag, cls, txt) {
    var n = document.createElement(tag);
    if (cls) n.className = cls;
    if (txt !== undefined && txt !== null) n.textContent = txt;
    return n;
  }

  function hex(v, digits) {
    var s = (v >>> 0).toString(16).toUpperCase();
    while (s.length < (digits || 2)) s = "0" + s;
    return "0x" + s;
  }

  function bin(v, digits) {
    var s = (v >>> 0).toString(2);
    while (s.length < (digits || 8)) s = "0" + s;
    return s;
  }

  function groupBin(s) {
    var out = [];
    for (var i = 0; i < s.length; i += 4) out.push(s.substr(i, 4));
    return out.join(" ");
  }

  function mount(target) {
    return typeof target === "string" ? document.getElementById(target) : target;
  }

  function shell(host, title) {
    var box = el("div", "mod");
    if (title) box.appendChild(el("div", "mod-title", title));
    host.appendChild(box);
    return box;
  }

  /* ============================================================
     MODULE: bitfield
     Click individual bits, watch the value assemble.
     Used for: GDT access byte, page table flags, PCI address, DCR, CR.
     ============================================================ */

  function bitfield(target, opt) {
    var host = mount(target);
    if (!host) return;
    var box = shell(host, opt.title || "Interactive, click the bits");

    var width = opt.width || 8;
    var value = opt.value || 0;
    var fields = opt.fields || [];   // {bit, name, desc} or {bits:[hi,lo], name, desc}
    var locked = opt.locked || [];

    if (opt.intro) {
      var p = el("div");
      p.style.fontSize = "0.9rem";
      p.style.color = "#b9c2cc";
      p.style.marginBottom = "12px";
      p.textContent = opt.intro;
      box.appendChild(p);
    }

    var bitsRow = el("div", "bf-bits");
    var out = el("div", "bf-out");
    var legend = el("div", "bf-legend");

    var cells = [];
    for (var i = width - 1; i >= 0; i--) {
      (function (bitIndex) {
        var c = el("div", "bf-bit");
        var idx = el("div", "idx", String(bitIndex));
        var v = el("div", "v", "0");
        c.appendChild(idx);
        c.appendChild(v);
        if (locked.indexOf(bitIndex) !== -1) c.classList.add("locked");
        c.addEventListener("click", function () {
          if (locked.indexOf(bitIndex) !== -1) return;
          value = (value ^ (1 << bitIndex)) >>> 0;
          render();
        });
        cells[bitIndex] = { node: c, val: v };
        bitsRow.appendChild(c);
      })(i);
    }

    var hexCell = el("div");
    hexCell.appendChild(el("span", null, "hex"));
    var hexB = el("b");
    hexCell.appendChild(hexB);

    var binCell = el("div");
    binCell.appendChild(el("span", null, "binary"));
    var binB = el("b");
    binCell.appendChild(binB);

    var decCell = el("div");
    decCell.appendChild(el("span", null, "decimal"));
    var decB = el("b");
    decCell.appendChild(decB);

    out.appendChild(hexCell);
    out.appendChild(binCell);
    out.appendChild(decCell);

    var legendRows = fields.map(function (f) {
      var row = el("div", "lg");
      var label = f.bits ? ("bit " + f.bits[0] + "-" + f.bits[1]) : ("bit " + f.bit);
      var nm = el("div", "nm", label);
      var ds = el("div", "ds", f.name + ": " + f.desc);
      row.appendChild(nm);
      row.appendChild(ds);
      row.addEventListener("click", function () {
        if (f.bits) {
          for (var b = f.bits[1]; b <= f.bits[0]; b++) {
            if (locked.indexOf(b) === -1) value = (value ^ (1 << b)) >>> 0;
          }
        } else if (locked.indexOf(f.bit) === -1) {
          value = (value ^ (1 << f.bit)) >>> 0;
        }
        render();
      });
      row.style.cursor = "pointer";
      legend.appendChild(row);
      return { node: row, f: f };
    });

    var note = null;
    if (opt.match) {
      note = el("div", "hint");
      box.appendChild(bitsRow);
      box.appendChild(out);
      box.appendChild(note);
      box.appendChild(legend);
    } else {
      box.appendChild(bitsRow);
      box.appendChild(out);
      box.appendChild(legend);
    }

    function render() {
      for (var b = 0; b < width; b++) {
        var on = !!(value & (1 << b));
        cells[b].val.textContent = on ? "1" : "0";
        cells[b].node.classList.toggle("on", on);
      }
      hexB.textContent = hex(value, Math.ceil(width / 4));
      binB.textContent = groupBin(bin(value, width));
      decB.textContent = String(value >>> 0);

      legendRows.forEach(function (r) {
        var active;
        if (r.f.bits) {
          active = false;
          for (var b = r.f.bits[1]; b <= r.f.bits[0]; b++) if (value & (1 << b)) active = true;
        } else {
          active = !!(value & (1 << r.f.bit));
        }
        r.node.classList.toggle("act", active);
      });

      if (note) {
        var m = opt.match[String(value)];
        note.textContent = m ? ("That is " + hex(value, Math.ceil(width / 4)) + ": " + m)
                             : "Keep going, no named value matches yet.";
        note.style.color = m ? "#4ade80" : "#8b949e";
      }
    }

    render();
  }

  /* ============================================================
     MODULE: packBuilder
     Shows one wide value split into the byte-sized pieces that
     actually get written to hardware. The "why >> 8 and & 0xFF" module.
     ============================================================ */

  function packBuilder(target, opt) {
    var host = mount(target);
    if (!host) return;
    var box = shell(host, opt.title || "Interactive, change the value");

    if (opt.intro) {
      var p = el("div");
      p.style.fontSize = "0.9rem";
      p.style.color = "#b9c2cc";
      p.style.marginBottom = "12px";
      p.textContent = opt.intro;
      box.appendChild(p);
    }

    var src = el("div", "pk-src");
    var lab = el("label", null, opt.label || "value:");
    var inp = el("input");
    inp.type = "text";
    inp.value = opt.value !== undefined ? String(opt.value) : "1514";
    src.appendChild(lab);
    src.appendChild(inp);
    var srcInfo = el("span");
    srcInfo.style.marginLeft = "12px";
    srcInfo.style.color = "#8b949e";
    srcInfo.style.fontSize = "0.85rem";
    src.appendChild(srcInfo);
    box.appendChild(src);

    var flow = el("div", "pk-flow");
    box.appendChild(flow);

    var cells = (opt.parts || []).map(function (part) {
      var c = el("div", "pk-cell");
      c.appendChild(el("div", "lbl", part.label));
      var val = el("div", "val", "");
      var b = el("div", "bin", "");
      var e = el("div", "expr", part.expr);
      c.appendChild(val);
      c.appendChild(b);
      c.appendChild(e);
      flow.appendChild(c);
      return { val: val, bin: b, part: part };
    });

    if (opt.hint) box.appendChild(el("div", "hint", opt.hint));

    function parse(s) {
      s = (s || "").trim();
      if (/^0x/i.test(s)) return parseInt(s, 16) || 0;
      return parseInt(s, 10) || 0;
    }

    function render() {
      var v = parse(inp.value) >>> 0;
      var maxBits = opt.srcBits || 16;
      srcInfo.textContent = hex(v, Math.ceil(maxBits / 4)) + "  =  " + groupBin(bin(v, maxBits));
      cells.forEach(function (c) {
        var got = c.part.calc(v) >>> 0;
        c.val.textContent = hex(got, 2);
        c.bin.textContent = bin(got, 8);
      });
    }

    inp.addEventListener("input", render);
    render();
  }

  /* ============================================================
     MODULE: memMap
     A clickable strip of regions. Used for the VGA buffer, kernel
     sections, card RAM pages, the heap block layout.
     ============================================================ */

  function memMap(target, opt) {
    var host = mount(target);
    if (!host) return;
    var box = shell(host, opt.title || "Interactive, click a region");

    var strip = el("div", "mm-strip");
    var detail = el("div", "mm-detail");
    box.appendChild(strip);
    box.appendChild(detail);
    if (opt.hint) box.appendChild(el("div", "hint", opt.hint));

    var segs = opt.segments || [];
    var nodes = segs.map(function (s, i) {
      var n = el("div", "mm-seg", s.label);
      n.style.flex = (s.weight || 1) + " 1 0";
      if (s.color) n.style.background = s.color;
      n.addEventListener("click", function () { pick(i); });
      strip.appendChild(n);
      return n;
    });

    function pick(i) {
      nodes.forEach(function (n, j) { n.classList.toggle("sel", i === j); });
      detail.innerHTML = "";
      var s = segs[i];
      detail.appendChild(el("div", "h", s.addr ? (s.label + "  " + s.addr) : s.label));
      detail.appendChild(el("div", "b", s.desc));
    }

    pick(opt.start || 0);
  }

  /* ============================================================
     MODULE: stepper
     Step through a sequence. Optional per step: code line, state values.
     Used for boot, interrupt dispatch, ARP exchange, malloc/free, reset.
     ============================================================ */

  function stepper(target, opt) {
    var host = mount(target);
    if (!host) return;
    var box = shell(host, opt.title || "Interactive, step through it");

    var steps = opt.steps || [];
    var at = 0;

    var bar = el("div", "st-bar");
    var pips = steps.map(function () {
      var p = el("div", "st-pip");
      bar.appendChild(p);
      return p;
    });

    var body = el("div", "st-body");
    var nNum = el("div", "st-n");
    var nHead = el("div", "st-h");
    var nText = el("div", "st-t");
    var nCode = el("div", "st-code");
    var nState = el("div", "st-state");
    body.appendChild(nNum);
    body.appendChild(nHead);
    body.appendChild(nText);
    body.appendChild(nCode);
    body.appendChild(nState);

    var nav = el("div", "st-nav");
    var back = el("button", "btn", "Back");
    var next = el("button", "btn primary", "Next step");
    var reset = el("button", "btn", "Restart");
    var pos = el("div", "pos");
    nav.appendChild(back);
    nav.appendChild(next);
    nav.appendChild(reset);
    nav.appendChild(pos);

    box.appendChild(bar);
    box.appendChild(body);
    box.appendChild(nav);
    if (opt.hint) box.appendChild(el("div", "hint", opt.hint));

    back.addEventListener("click", function () { if (at > 0) { at--; render(); } });
    next.addEventListener("click", function () { if (at < steps.length - 1) { at++; render(); } });
    reset.addEventListener("click", function () { at = 0; render(); });

    function render() {
      var s = steps[at];
      nNum.textContent = "Step " + (at + 1) + " of " + steps.length;
      nHead.textContent = s.head;
      nText.textContent = s.text || "";
      if (s.code) { nCode.style.display = "block"; nCode.textContent = s.code; }
      else { nCode.style.display = "none"; }

      nState.innerHTML = "";
      if (s.state) {
        Object.keys(s.state).forEach(function (k) {
          var w = el("div");
          w.appendChild(el("i", null, k + " = "));
          w.appendChild(el("b", null, s.state[k]));
          nState.appendChild(w);
        });
      }

      pips.forEach(function (p, i) {
        p.classList.toggle("done", i < at);
        p.classList.toggle("now", i === at);
      });
      back.disabled = at === 0;
      next.disabled = at === steps.length - 1;
      pos.textContent = (at + 1) + "/" + steps.length;
    }

    render();
  }

  /* ============================================================
     MODULE: ringViz
     Ring buffer with head/tail. Used for the NE2000 receive ring.
     ============================================================ */

  function ringViz(target, opt) {
    var host = mount(target);
    if (!host) return;
    var box = shell(host, opt.title || "Interactive, move the pointers");

    var first = opt.first || 0x46;
    var last = opt.last || 0x60;      // exclusive, matches PSTOP
    var count = last - first;

    var write = 1;   // card's next write page, offset from first
    var read = 0;    // our boundary, offset from first
    var full = {};   // offset -> true when a packet sits there

    var wrap = el("div", "rg-wrap");
    var grid = el("div", "rg-grid");
    var side = el("div", "rg-side");
    wrap.appendChild(grid);
    wrap.appendChild(side);
    box.appendChild(wrap);

    var cells = [];
    for (var i = 0; i < count; i++) {
      var c = el("div", "rg-cell", hex(first + i, 2).replace("0x", ""));
      grid.appendChild(c);
      cells.push(c);
    }

    var kvCur = el("div", "kv");
    var kvBnry = el("div", "kv");
    var kvState = el("div", "kv");
    side.appendChild(kvCur);
    side.appendChild(kvBnry);
    side.appendChild(kvState);

    var btns = el("div", "row");
    btns.style.marginTop = "12px";
    var bRecv = el("button", "btn", "Packet arrives");
    var bRead = el("button", "btn primary", "We read one");
    var bReset = el("button", "btn", "Reset");
    btns.appendChild(bRecv);
    btns.appendChild(bRead);
    btns.appendChild(bReset);
    side.appendChild(btns);

    var log = el("div", "rg-log");
    side.appendChild(log);
    if (opt.hint) box.appendChild(el("div", "hint", opt.hint));

    function say(m) {
      var line = el("div", null, "> " + m);
      log.insertBefore(line, log.firstChild);
    }

    bRecv.addEventListener("click", function () {
      var nextW = (write + 1) % count;
      if (nextW === read) { say("ring full, card would drop the packet"); return; }
      full[write] = true;
      say("card wrote a packet to page " + hex(first + write, 2));
      write = nextW;
      render();
    });

    bRead.addEventListener("click", function () {
      var nextR = (read + 1) % count;
      if (nextR === write) { say("nothing new, BNRY+1 equals CURPAG"); return; }
      delete full[nextR];
      read = nextR;
      say("read the packet, advanced BNRY to " + hex(first + read, 2));
      render();
    });

    bReset.addEventListener("click", function () {
      write = 1; read = 0; full = {}; log.innerHTML = "";
      render();
    });

    function render() {
      cells.forEach(function (c, i) {
        c.classList.toggle("full", !!full[i]);
        c.innerHTML = hex(first + i, 2).replace("0x", "");
        if (i === read) { var h = el("div", "mk h", "B"); c.appendChild(h); }
        if (i === write) { var t = el("div", "mk t", "C"); c.appendChild(t); }
      });
      kvBnry.innerHTML = "";
      kvBnry.appendChild(el("i", null, "BNRY (we read to) = "));
      kvBnry.appendChild(el("b", null, hex(first + read, 2)));
      kvCur.innerHTML = "";
      kvCur.appendChild(el("i", null, "CURPAG (card writes) = "));
      kvCur.appendChild(el("b", null, hex(first + write, 2)));
      var waiting = ((write - read) + count) % count - 1;
      kvState.innerHTML = "";
      kvState.appendChild(el("i", null, "packets waiting = "));
      kvState.appendChild(el("b", null, String(Math.max(0, waiting))));
    }

    render();
  }

  /* ============================================================
     MODULE: codeWalk
     Real code with clickable annotations. Hover or click a note and
     the matching lines light up.
     ============================================================ */

  function codeWalk(target, opt) {
    var host = mount(target);
    if (!host) return;
    var box = shell(host, opt.title || "Interactive, click a note to highlight the code");

    var wrap = el("div", "cw-wrap");
    var codeBox = el("div", "cw-code");
    var notesBox = el("div", "cw-notes");
    wrap.appendChild(codeBox);
    wrap.appendChild(notesBox);
    box.appendChild(wrap);

    var lines = (opt.code || "").replace(/\t/g, "    ").split("\n");
    var lineNodes = lines.map(function (txt, i) {
      var row = el("div", "cw-line");
      row.appendChild(el("span", "ln", String(i + 1)));
      row.appendChild(el("span", "tx", txt));
      codeBox.appendChild(row);
      return row;
    });

    var noteNodes = (opt.notes || []).map(function (n) {
      var node = el("div", "cw-note");
      var range = n.lines[0] === n.lines[1] ? ("line " + n.lines[0]) : ("lines " + n.lines[0] + "-" + n.lines[1]);
      node.appendChild(el("div", "nl", range));
      node.appendChild(el("div", "nb", n.text));
      node.addEventListener("mouseenter", function () { light(n); });
      node.addEventListener("click", function () { light(n, node); });
      notesBox.appendChild(node);
      return { node: node, n: n };
    });

    function light(n, clicked) {
      lineNodes.forEach(function (row, i) {
        var ln = i + 1;
        row.classList.toggle("hot", ln >= n.lines[0] && ln <= n.lines[1]);
      });
      noteNodes.forEach(function (x) { x.node.classList.toggle("hot", x.n === n); });
      if (clicked) {
        var target2 = lineNodes[n.lines[0] - 1];
        if (target2 && target2.scrollIntoView) {
          // keep it gentle, no page jump on small screens
        }
      }
    }

    // mark taggable lines
    (opt.notes || []).forEach(function (n) {
      for (var ln = n.lines[0]; ln <= n.lines[1]; ln++) {
        if (lineNodes[ln - 1]) lineNodes[ln - 1].classList.add("tag");
      }
    });

    if (noteNodes.length) light(noteNodes[0].n);
    if (opt.hint) box.appendChild(el("div", "hint", opt.hint));
  }

  /* ============================================================
     MODULE: hexDump
     Real bytes with a field breakdown. Used for the captured ARP frame.
     ============================================================ */

  function hexDump(target, opt) {
    var host = mount(target);
    if (!host) return;
    var box = shell(host, opt.title || "Interactive, hover the bytes");

    if (opt.intro) {
      var p = el("div");
      p.style.fontSize = "0.9rem";
      p.style.color = "#b9c2cc";
      p.style.marginBottom = "12px";
      p.textContent = opt.intro;
      box.appendChild(p);
    }

    var dump = el("div", "hx");
    var legend = el("hx-legend");
    legend = el("div", "hx-legend");
    box.appendChild(dump);
    box.appendChild(legend);

    var bytes = opt.bytes || [];
    var fields = opt.fields || [];

    var byteNodes = bytes.map(function (b, i) {
      var n = el("span", "hx-b", b);
      n.addEventListener("mouseenter", function () { hit(i); });
      dump.appendChild(n);
      if ((i + 1) % 16 === 0) dump.appendChild(el("br"));
      else if ((i + 1) % 8 === 0) dump.appendChild(document.createTextNode("  "));
      return n;
    });

    var legendNodes = fields.map(function (f) {
      var row = el("div", "hx-lg");
      row.appendChild(el("div", "n", f.name));
      row.appendChild(el("div", "d", f.desc));
      row.addEventListener("mouseenter", function () { show(f); });
      legend.appendChild(row);
      return { node: row, f: f };
    });

    function show(f) {
      byteNodes.forEach(function (n, i) {
        n.classList.toggle("hot", i >= f.from && i <= f.to);
      });
      legendNodes.forEach(function (x) { x.node.classList.toggle("hot", x.f === f); });
    }

    function hit(i) {
      var f = fields.filter(function (x) { return i >= x.from && i <= x.to; })[0];
      if (f) show(f);
    }

    if (fields.length) show(fields[0]);
    if (opt.hint) box.appendChild(el("div", "hint", opt.hint));
  }

  /* ============================================================
     MODULE: quiz
     One question, instant feedback, always explains why.
     ============================================================ */

  function quiz(target, opt) {
    var host = mount(target);
    if (!host) return;
    var box = shell(host, opt.title || "Quick check");

    box.appendChild(el("div", "qz-q", opt.question));
    var opts = el("div", "qz-opts");
    var exp = el("div", "qz-exp");
    box.appendChild(opts);
    box.appendChild(exp);

    var answered = false;
    (opt.options || []).forEach(function (o, i) {
      var b = el("button", "qz-opt", o.text);
      b.addEventListener("click", function () {
        if (answered) return;
        answered = true;
        var correct = !!o.correct;
        b.classList.add(correct ? "right" : "wrong");
        if (!correct) {
          // also reveal the right one
          Array.prototype.forEach.call(opts.children, function (btn, j) {
            if (opt.options[j].correct) btn.classList.add("right");
          });
        }
        exp.textContent = o.why || opt.explain || "";
        exp.classList.add("show");
      });
      opts.appendChild(b);
    });
  }

  /* ============================================================
     MODULE: terminal
     Replays real OS output, typed out. Not an emulator, a recording.
     ============================================================ */

  function terminal(target, opt) {
    var host = mount(target);
    if (!host) return;
    var box = shell(host, opt.title || "Recorded output from a real run");

    var scr = el("div", "tm");
    box.appendChild(scr);
    var row = el("div", "row");
    row.style.marginTop = "10px";
    var play = el("button", "btn primary", "Play");
    row.appendChild(play);
    box.appendChild(row);
    if (opt.hint) box.appendChild(el("div", "hint", opt.hint));

    var text = opt.text || "";
    var timer = null;

    function run() {
      if (timer) clearInterval(timer);
      var i = 0;
      scr.textContent = "";
      play.disabled = true;
      timer = setInterval(function () {
        if (i >= text.length) {
          clearInterval(timer);
          timer = null;
          play.disabled = false;
          play.textContent = "Play again";
          return;
        }
        // type a few chars per tick so it is not painfully slow
        scr.textContent += text.substr(i, 2);
        i += 2;
        scr.scrollTop = scr.scrollHeight;
      }, opt.speed || 18);
    }

    play.addEventListener("click", run);
    scr.textContent = text.split("\n").slice(0, 2).join("\n");
  }

  /* ============================================================
     page chrome: sidebar + nav, built from one shared list
     ============================================================ */

  var PAGES = [
    { group: "Start here" },
    { id: "index", file: "index.html", title: "Overview", desc: "What this guide is and how to use it" },

    { group: "Foundations" },
    { id: "bits", file: "bits.html", title: "Bits, bytes and hex", desc: "Reading hex, masks, shifts, and why hardware talks in bits" },
    { id: "flags", file: "flags.html", title: "Flags and bit fields", desc: "Packing many yes/no answers into one byte" },
    { id: "endian", file: "endian.html", title: "Byte order", desc: "Little endian, big endian, and network order" },
    { id: "cpp", file: "cpp.html", title: "C++ without a system", desc: "Freestanding code, volatile, structs, pointers, packed" },

    { group: "Getting on screen" },
    { id: "boot", file: "boot.html", title: "Boot and the linker", desc: "GRUB, multiboot, sections, the stack, kernel_end" },
    { id: "vga", file: "vga.html", title: "VGA text mode", desc: "Writing characters straight into memory at 0xB8000" },

    { group: "Talking to hardware" },
    { id: "ports", file: "ports.html", title: "Port I/O", desc: "inb, outb, inl, outl and how a port differs from memory" },
    { id: "keyboard", file: "keyboard.html", title: "Keyboard and scancodes", desc: "Scancodes, make and break codes, shift state" },
    { id: "gdt", file: "gdt.html", title: "The GDT", desc: "Segments, descriptors, and that awkward split layout" },
    { id: "interrupts", file: "interrupts.html", title: "Interrupts and the IDT", desc: "Gates, stubs, iret, and what the CPU pushes" },
    { id: "pic", file: "pic.html", title: "The PIC", desc: "Remapping IRQs, masks, and end of interrupt" },
    { id: "time", file: "time.html", title: "Timer and clock", desc: "PIT divisors and reading the RTC in BCD" },

    { group: "Managing memory" },
    { id: "pmm", file: "pmm.html", title: "Physical memory", desc: "Frames, a bitmap, and GRUB's memory map" },
    { id: "paging", file: "paging.html", title: "Paging", desc: "Virtual addresses, two level tables, identity mapping" },
    { id: "heap", file: "heap.html", title: "The heap", desc: "Block headers, first fit, splitting and coalescing" },

    { group: "Getting online" },
    { id: "pci", file: "pci.html", title: "The PCI bus", desc: "Config space, the address word, vendor and device IDs" },
    { id: "ne2000", file: "ne2000.html", title: "The NE2000 card", desc: "Register pages, remote DMA, card RAM, send and receive" },
    { id: "ethernet", file: "ethernet.html", title: "Ethernet frames", desc: "MAC addresses, the 14 byte header, EtherType" },
    { id: "arp", file: "arp.html", title: "ARP", desc: "Turning an IP address into a MAC address" }
  ];

  function buildChrome(currentId) {
    var layout = document.querySelector(".layout");
    if (!layout) return;

    var side = el("nav", "sidebar");
    var brand = el("div", "brand");
    brand.appendChild(document.createTextNode("Leveret"));
    brand.appendChild(el("span", null, "OS"));
    brand.appendChild(document.createTextNode(" guide"));
    side.appendChild(brand);

    PAGES.forEach(function (p) {
      if (p.group) {
        side.appendChild(el("div", "group", p.group));
        return;
      }
      var a = el("a", "item", p.title);
      a.href = p.file;
      if (p.id === currentId) a.classList.add("active");
      side.appendChild(a);
    });

    layout.insertBefore(side, layout.firstChild);

    var btn = el("button", "menu-btn", "Contents");
    btn.addEventListener("click", function () { side.classList.toggle("open"); });
    document.body.appendChild(btn);

    // prev / next
    var flat = PAGES.filter(function (p) { return !p.group; });
    var idx = -1;
    flat.forEach(function (p, i) { if (p.id === currentId) idx = i; });
    if (idx >= 0) {
      var inner = document.querySelector(".inner");
      if (inner) {
        var nav = el("div", "pnav");
        if (idx > 0) {
          var a1 = el("a", null, "\u2190 " + flat[idx - 1].title);
          a1.href = flat[idx - 1].file;
          nav.appendChild(a1);
        } else nav.appendChild(el("span"));
        if (idx < flat.length - 1) {
          var a2 = el("a", null, flat[idx + 1].title + " \u2192");
          a2.href = flat[idx + 1].file;
          nav.appendChild(a2);
        } else nav.appendChild(el("span"));
        inner.appendChild(nav);
      }
    }
  }

  function indexCards(target) {
    var host = mount(target);
    if (!host) return;
    var wrap = el("div", "cards");
    PAGES.filter(function (p) { return !p.group && p.id !== "index"; }).forEach(function (p) {
      var a = el("a", "card");
      a.href = p.file;
      a.appendChild(el("div", "k", p.id));
      a.appendChild(el("div", "t", p.title));
      a.appendChild(el("div", "d", p.desc));
      wrap.appendChild(a);
    });
    host.appendChild(wrap);
  }

  /* ============================================================
     export
     ============================================================ */

  window.Guide = {
    bitfield: bitfield,
    packBuilder: packBuilder,
    memMap: memMap,
    stepper: stepper,
    ringViz: ringViz,
    codeWalk: codeWalk,
    hexDump: hexDump,
    quiz: quiz,
    terminal: terminal,
    chrome: buildChrome,
    indexCards: indexCards,
    pages: PAGES
  };
})();
