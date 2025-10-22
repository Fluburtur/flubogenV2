class BlurPreview extends Canvas {
	constructor(canvas) {
		super(canvas);

		this.resizeCanvas(
			MATRIX.width*(LED_SIZE + LED_GAP) + LED_GAP,
			MATRIX.height*(LED_SIZE + LED_GAP) + LED_GAP
		);

		this.frame = LED_MATRIX.map(x => [...x]);
		this.previewFrame = LED_MATRIX.map(x => [...x]);
		this.matrixCanvas = q("<canvas>");
		this.matrixCanvas.width = MATRIX.width;
		this.matrixCanvas.height = MATRIX.height;
		this.matrixCanvasCtx = this.matrixCanvas.getContext("2d");
		this.scalingCanvas = q(/[blurModal]/).q("<canvas hidden>");
	}

	setFrame(frame, draw=true) {
		for (const coord of led_coordinates) {
			const [x, y] = coord.map(x => x - 1);
			this.frame[y][x] = frame[y][x];
			this.previewFrame[y][x] = frame[y][x];
		}
		this.update(draw);
	}

	update(draw=true) {
		const scaling = +q(/[blurScaling]/).value;
		const amount = +q(/[blurAmount]/).value;
		this.scalingCanvas.width = this.matrixCanvas.width*scaling;
		this.scalingCanvas.height = this.matrixCanvas.height*scaling;
		let scalingCanvasCtx = this.scalingCanvas.getContext("2d");
		scalingCanvasCtx.filter = `blur(${amount}px)`;
		scalingCanvasCtx.imageSmoothingEnabled = false;
		let matrixImageData = Array(this.matrixCanvas.width*this.matrixCanvas.height*4).fill(255);

		for (var y = 0; y < this.matrixCanvas.height; y++) {
			for (var x = 0; x < this.matrixCanvas.width; x++) {
				const color = this.frame[y][x];

				for (var i = 0; i < color.length; i++)
					matrixImageData[(y*this.matrixCanvas.width + x)*4 + i] = color[i];
			}
		}

		let imageData = this.matrixCanvasCtx.createImageData(this.matrixCanvas.width, this.matrixCanvas.height);
		imageData.data.set(new Uint8ClampedArray(matrixImageData));
		this.matrixCanvasCtx.putImageData(imageData, 0, 0);
		scalingCanvasCtx.drawImage(this.matrixCanvas, 0, 0, this.matrixCanvas.width, this.matrixCanvas.height, 0, 0, this.scalingCanvas.width, this.scalingCanvas.height);
		this.matrixCanvasCtx.drawImage(this.scalingCanvas, 0, 0, this.scalingCanvas.width, this.scalingCanvas.height, 0, 0, this.matrixCanvas.width, this.matrixCanvas.height);
		imageData = this.matrixCanvasCtx.getImageData(0, 0, this.matrixCanvas.width, this.matrixCanvas.height);
		matrixImageData = imageData.data;


		for (const coord of led_coordinates) {
			const [x, y] = coord.map(x => x - 1);
			let color = Array(3);

			for (var i = 0; i < color.length; i++)
				color[i] = matrixImageData[(y*this.matrixCanvas.width + x)*4 + i];

			this.previewFrame[y][x] = color;
		}

		if (draw)
			this.draw();
	}

	blurFrames(frames) {
		let blurredFrames = [];

		for (const frame of frames) {
			this.setFrame(frame, false);
			blurredFrames.push(this.previewFrame.map(x => [...x]));
		}

		return blurredFrames;
	}

	draw() {
		this.clear()
		let holeX = MATRIX.hole.origin.x;
		let holeY = MATRIX.hole.origin.y;

		for (let i = 0; i < this.previewFrame.length; i++) {
			for (let j = 0; j < this.previewFrame[i].length; j++) {
				if ((j < holeX || MATRIX.hole.width + holeX - 1 < j) ||
					(i < holeY || MATRIX.hole.height + holeY - 1 < i)) {
					// draw background of the rectangle
					this.fill(30, 30, 30);
					this.rect(j * (LED_GAP + LED_SIZE), i * (LED_GAP + LED_SIZE), LED_SIZE + LED_GAP*2, LED_SIZE + LED_GAP*2);
					this.fill(...this.previewFrame[i][j]);
					this.rect(j * (LED_GAP + LED_SIZE) + LED_GAP, i * (LED_GAP + LED_SIZE) + LED_GAP, LED_SIZE, LED_SIZE);
				}
			}
		}
	}
}