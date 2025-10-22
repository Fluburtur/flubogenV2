const MATRIX = {
	width: 36,
	height: 10,
	hole: {
		width: 16,
		height: 3,
		origin: {
			x: 10,
			y: 0
		}
	}
};

const LED_SIZE = 25;  // size of the led square in pixels
const LED_GAP = 4;  // gap between leds in pixels
const LED_MATRIX = Array(MATRIX.height).fill().map(() => Array(MATRIX.width).fill().map(x => [0, 0, 0]))  // led[y][x] -> [R, G, B] NOTE: the position of y and x is correct
const defaultAnimationNames = ["Idle animation", ...([1, 2, 3].map(i => "Random animation " + i)), "Startup animation"]

class Animator extends Canvas {
	constructor(canvas) {
		super(canvas);

		this.resizeCanvas(
			MATRIX.width*(LED_SIZE + LED_GAP) + LED_GAP,
			MATRIX.height*(LED_SIZE + LED_GAP) + LED_GAP
		);

		this.animationIndex = 0;
		this.frameIndex = 0;
		this.animations = [];
		this.previewInterval = null;
		this.drawWiring = false;

		defaultAnimationNames.map(
			animation_name => this.animations.push({
				name: animation_name,
				FPS: 1,
				loop: false,
				frames: [LED_MATRIX.map(x => [...x])]
			})
		);

		this.clear();
		this.draw();
	}

	draw() {
		if (this.frameIndex > this.animations[this.animationIndex].frames.length - 1)
			return;

		this.clear();
		let leds = [...this.animations[this.animationIndex].frames[this.frameIndex]];
		let holeX = MATRIX.hole.origin.x;
		let holeY = MATRIX.hole.origin.y;

		for (let i = 0; i < leds.length; i++) {
			for (let j = 0; j < leds[i].length; j++) {
				if ((j < holeX || MATRIX.hole.width + holeX - 1 < j) ||
					(i < holeY || MATRIX.hole.height + holeY - 1 < i)) {
					// Draw background of dot (grey border)
					this.fill(30, 30, 30);
					this.rect(j * (LED_GAP + LED_SIZE), i * (LED_GAP + LED_SIZE), LED_SIZE + LED_GAP*2, LED_SIZE + LED_GAP*2);

					// Draw color of dot, cursor color has priority over set color
					if (Math.floor(this.mouseX/(LED_GAP + LED_SIZE)) === j && Math.floor(this.mouseY/(LED_GAP + LED_SIZE)) === i && this.previewInterval === null) {
						this.fill(...parseColor(q(/input[type="color"]/).value));
					} else
						this.fill(...leds[i][j]);
					this.rect(j * (LED_GAP + LED_SIZE) + LED_GAP, i * (LED_GAP + LED_SIZE) + LED_GAP, LED_SIZE, LED_SIZE);
				}
			}
		}
		
		// Draw wiring
		if(this.drawWiring) {
			let offset = (LED_GAP + LED_SIZE) / 2;
			let [px, py] = led_coordinates[0].map(v => offset + (v - 1) * (LED_GAP + LED_SIZE));
			let color_ratio = 255.0 / led_coordinates.length;
			for (let i = 1; i < led_coordinates.length; i++) {
				let [x, y] = led_coordinates[i].map(v => offset + (v - 1) * (LED_GAP + LED_SIZE));

				this.line(px, py, x, y, i * color_ratio, 255 - (i * color_ratio), 0);
				[px, py] = [x, y];
			}
		}
	}

	onMouseDown() {
		if(this.previewInterval !== null)
			return;

		var animation = this.animations[this.animationIndex];
		var leds = animation.frames[this.frameIndex];
		let ledX = Math.floor(this.mouseX/(LED_GAP + LED_SIZE));
		let ledY = Math.floor(this.mouseY/(LED_GAP + LED_SIZE));

		if (ledX < 0 || ledY < 0 || ledY > leds.length - 1 || ledX > leds[0].length - 1)
			return;

		if (this.mousePressed === 1) {  // left mouse button
			if (!compareColors([0, 0, 0], leds[ledY][ledX])) {  // don't replace black
				if(q(/input[name="replaceAllColors"]/).checked) {
					for (let i = 0; i < animation.frames.length; i++)  // replace color in each frame
						animation.frames[i] = replaceColor(leds[ledY][ledX], parseColor(q(/input[type="color"]/).value), animation.frames[i]);
				} else if(q(/input[name="replaceColors"]/).checked)
					animation.frames[this.frameIndex] = replaceColor(leds[ledY][ledX], parseColor(q(/input[type="color"]/).value), leds);
			}

			// mirroring of drawing
			if(q(/input[name="mirroring"]/).checked)
				leds[ledY][leds[ledY].length - ledX - 1] = parseColor(q(/input[type="color"]/).value);
			
			leds[ledY][ledX] = parseColor(q(/input[type="color"]/).value);
		} else if (this.mousePressed === 2) {  // right mouse button
			if(q(/input[name="pickColors"]/).checked)
				q(/input[type="color"]/).value = parseColor(leds[ledY][ledX]);
			else {  // remove color
				leds[ledY][ledX] = [0, 0, 0];
				
				// mirroring color removing
				if(q(/input[name="mirroring"]/).checked)
					leds[ledY][leds[ledY].length - ledX - 1] = [0, 0, 0];
			}
		}

		this.draw();
	}

	startPreview() {
		const frames = this.animations[this.animationIndex].frames;
		const FPS = this.animations[this.animationIndex].FPS;
		if (this.previewInterval !== null)  // don't start preview when it's already started
			return;

		q(/[navigationRight] button, [navigationRight] input/g).map(x => x.setAttribute("disabled", ""));
		q(/[navigationLeft] button, [navigationLeft] select/g).map(x => x.setAttribute("disabled", ""));
		q(/[frameSlider]/).setAttribute("disabled", "");
		this.previewInterval = setInterval(() => {
			requestAnimationFrame(this.draw.bind(this));
			this.frameIndex++;
			if (this.frameIndex > frames.length - 1) {
				this.stopPreview();
				return;
			}
			q(/[frameNavigation] p/).textContent = (this.frameIndex + 1) + "/" + frames.length;
			q(/p[timeCode]/).textContent = timeCode(this.frameIndex, FPS) + "/" + timeCode(frames.length - 1, FPS);
			q(/[frameSlider]/).value = this.frameIndex;
		}, 1000 / FPS);
	}

	stopPreview() {
		if (this.previewInterval === null)  // don't stop non-running preview
			return;

		this.frameIndex = 0;
		q(/[navigationRight] button, [navigationRight] input/g).map(x => x.removeAttribute("disabled"));
		q(/[navigationLeft] button, [navigationLeft] select/g).map(x => x.removeAttribute("disabled"));
		q(/[frameSlider]/).removeAttribute("disabled");
		refreshNavigation();
		clearInterval(this.previewInterval);
		this.previewInterval = null;
		this.draw();
	}
}