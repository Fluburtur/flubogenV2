class Canvas {
	constructor(canvas) {
		if (!(canvas instanceof HTMLCanvasElement))
			throw "Constructor argument has to be canvas";

		this.canvas = canvas;
		this.width = canvas.width;
		this.height = canvas.height;
		this.ctx = canvas.getContext("2d");
		this.mouseX = -1;
		this.mouseY = -1;
		this.mousePressed = false;

		this.canvas.onmousemove = evt => {
			let rect = canvas.getBoundingClientRect();
			this.mouseX = evt.clientX - rect.left;
			this.mouseY = evt.clientY - rect.top;
			this.draw();
			if (this.mousePressed)
				this.onMouseDown();
		};
		this.canvas.onmouseleave = evt => {
			this.mouseX = -1;
			this.mouseY = -1;
			this.draw();
		};
		this.canvas.addEventListener("mousedown", (evt) => {
			// 1 - left, 2 - right
			this.mousePressed = evt.buttons;
			this.onMouseClick();
			this.onMouseDown();
			this.draw();
		});
		this.canvas.addEventListener("mouseup", () => {
			this.mousePressed = false;
			this.draw();
		});
	}

	// these functions can be overriden by child class
	draw() {}
	onMouseClick() {}
	onMouseDown() {}

	resizeCanvas(width, height) {
		this.canvas.width = width;
		this.canvas.height = height;
		this.width = this.canvas.width;
		this.height = this.canvas.height;
	}

	clear() {
		let color = this.ctx.fillStyle;
		this.ctx.fillStyle = "white";
		this.ctx.fillRect(0, 0, this.width, this.height);
		this.ctx.fillStyle = color;
	}

	fill(r, g, b) {
		this.ctx.fillStyle = `rgb(${r}, ${g}, ${b})`;
	}

	rect(x, y, width, height) {
		this.ctx.fillRect(x, y, width, height);
	}

	line(xa, ya, xb, yb, r, g, b) {
		this.ctx.beginPath();
		this.ctx.moveTo(xa, ya);
		this.ctx.lineTo(xb, yb);
		this.ctx.strokeStyle = `rgb(${r}, ${g}, ${b})`;
		this.ctx.stroke();
	}
}