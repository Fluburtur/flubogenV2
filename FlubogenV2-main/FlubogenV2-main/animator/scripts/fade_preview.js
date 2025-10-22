class FadePreview extends Canvas {
	constructor(canvas) {
		super(canvas);

		this.resizeCanvas(
			MATRIX.width*(LED_SIZE + LED_GAP) + LED_GAP,
			MATRIX.height*(LED_SIZE + LED_GAP) + LED_GAP
		);

		this.frame = LED_MATRIX.map(x => [...x]);
		this.fadedFrames = [this.frame];
		this.frameIndex = 0;
	}

	setFrame(frame, draw=true) {
		for (const coord of led_coordinates) {
			const [x, y] = coord.map(x => x - 1);
			this.frame[y][x] = frame[y][x];
		}
		this.update(draw);
	}

	update(draw=true) {
		const fadeLength = +q(/[fadeLength]/).value + 1;
		const fadeIn = !+q(/[fadeType]/).value;
		this.fadedFrames = Array(fadeLength);

		for (let i = 0; i < fadeLength; i++) {
			const mul = fadeIn ? i/(fadeLength - 1) : 1 - i/(fadeLength - 1);
			this.fadedFrames[i] = this.frame.map(
				row => row.map(
					color => color.map(x => x*mul)
				)
			);
		}

		if (this.frameIndex > this.fadedFrames.length - 1)
			this.frameIndex = this.fadedFrames.length - 1

		q(/[fadeModal] [fadeFrameSlider]/).setAttribute("max", this.fadedFrames.length);
		q(/[fadeModal] [fadeFrameSlider]/).value = this.frameIndex + 1;

		if (draw)
			this.draw();
	}

	fadeFrames(frames) {
		return this.fadedFrames;
	}

	draw() {
		this.clear()
		let holeX = MATRIX.hole.origin.x;
		let holeY = MATRIX.hole.origin.y;
		const frame = this.fadedFrames[this.frameIndex];

		for (let i = 0; i < frame.length; i++) {
			for (let j = 0; j < frame[i].length; j++) {
				if ((j < holeX || MATRIX.hole.width + holeX - 1 < j) ||
					(i < holeY || MATRIX.hole.height + holeY - 1 < i)) {
					// draw background of the rectangle
					this.fill(30, 30, 30);
					this.rect(j * (LED_GAP + LED_SIZE), i * (LED_GAP + LED_SIZE), LED_SIZE + LED_GAP*2, LED_SIZE + LED_GAP*2);
					this.fill(...frame[i][j]);
					this.rect(j * (LED_GAP + LED_SIZE) + LED_GAP, i * (LED_GAP + LED_SIZE) + LED_GAP, LED_SIZE, LED_SIZE);
				}
			}
		}
	}
}