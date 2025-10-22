const DEFAULT_COLOR_PALETTE = ["#9400d3", "#4b0082", "#0000ff", "#00ff00", "#ffff00", "#ff7f00", "#ff0000"];
let interval;   // this is used for timing of the gif preview
let animator, frameClipboard, animationClipboard, blurPreview, fadePreview;

document.addEventListener('contextmenu', event => event.preventDefault());  // disable context menu
document.addEventListener('keydown', evt => {
	if(![q(/[animationName]/), q(/[animationFPS]/)].map(x => x === document.activeElement).some(x => x)) {  // if none text input is active
		if(evt.keyCode === 37 && 0 < animator.frameIndex) {  // left arrow
			animator.frameIndex--;
			refreshNavigation();
		} else if(evt.keyCode === 39 && animator.frameIndex < animator.animations[animator.animationIndex].frames.length - 1) {  // right arrow
			animator.frameIndex++;
			refreshNavigation();
		}
	}
	animator.draw();
});

window.onload = () => {
	q(/[closeErrorMessage]/).onclick = () => q(/[errorBackground]/).style.display = "none";

	for(const color of DEFAULT_COLOR_PALETTE) {
		addColor(color);
	}

	animator = new Animator(q(/[mainCanvas]/));
	blurPreview = new BlurPreview(q(/[blurPreview]/));
	fadePreview = new FadePreview(q(/[fadePreview]/));
	createNavigation();
	refreshNavigation();
}

function parseColor(color) {
	let output;
	if (typeof color === "string") {
		output = [];
		for (let i = 0; i < 3; i++)
			output.push(parseInt(color.slice(1, 7).slice(i*2, i*2+2), 16));
	} else if (color instanceof Array) {
		output = "#";
		for (const n of color)
			output += n.toString(16).padStart(2, 0);
	}

	return output;
}

function addColor(x = q(/input[type="color"]/).value) {
	let color = q(/[palette]/).q("<div>");
	color.style["background-color"] = x;
	let text = color.q("<p>" + x);
	text.style.color = parseColor(parseColor(x).map(x => 255 - x))
	text.style.align = "center";

	let magic = (evt) => evt.target.localName === "div" ? evt.target : evt.target.parentElement  // always return div with color

	color.onclick = (evt) => q(/input[type="color"]/).value = parseColor(magic(evt).style["background-color"].match(/\d+/g).map(x => parseInt(x)));
	color.addEventListener('contextmenu', evt => magic(evt).parentElement.removeChild(magic(evt)));
}

function createNavigation() {
	q(/#canvasHolder/)
		.q(`<div navigation block>`)
			.q([`<div navigationLeft>`, `<div navigationCenter>`, `<div navigationRight>`]);

	q(/div[navigationLeft]/)
		.q([`<select animation no-flex>`,
			`<button moveAnimationUp disabled>Move animation up`,
			`<button moveAnimationDown disabled>Move animation down`,
			`<button createAnimation>Create animation`,
			`<button deleteAnimation disabled>Delete animation`,
			`<label for="showWiring"><input showWiring type="checkbox" name="showWiring"> Show LED wiring</label>`
		]);

	q(/div[navigationRight]/)
		.q(["<div frameNavigation>",
			`<p timeCode>00:00:00.00/00:00:00.00</p>`,
			`<input animationFPS type="number" min="1" max="60" size="5">`,
			`<button changeFPS>Change animation FPS`,
			`<button copyPreviousFrame disabled>Copy previous frame`,
			`<button addNextFrame>Add next frame`,
			`<button addStartFrame>Add frame to start`,
			`<button clearFrame>Clear frame`,
			`<button copyFrame>Copy frame`,
			`<button copyAnimation>Copy animation`,
			`<p>Duplicate count:`,
			`<input duplicateCount type="number" min="1" value="1" size="5">`,
			`<button duplicateFrame>Duplicate frame`,
			`<button pasteFrame disabled>Paste frame`,
			`<button pasteAnimation disabled>Paste animation`,
			`<button deleteFrame disabled>Delete frame`,
			`<button importImage>Import image as frame`,
			`<button importScrollingText>Import image as scrolling text`,
			`<button importGIF>Import GIF as frames`,
			`<button blur>Blur`,
			`<button fade>Fade`
		]);

	q(/div[navigationCenter]/)
		.q(`<div class="slidecontainer">`)
			.q(`<input frameSlider type="range" class="slider" min="0" step="1">`);

	q(/div[navigationCenter]/)
		.q(`<div previewControl>`)
			.q([`<button previewStart>▶`,
				`<button previewStop>⏹`
			])

	q(/div[navigationCenter]/)
		.q([`<p>Select color:</p>`,
			`<input type="color" value="#FFFFFF">`,
			`<button onclick="addColor()">Add color to palette</button>`,
			`<label for="pickColors"><input type="checkbox" name="pickColors"> Pick color with right click</label>`,
			`<label for="replaceColors"><input type="checkbox" name="replaceColors"> Replace colors</label>`,
			`<label for="replaceAllColors"><input type="checkbox" name="replaceAllColors"> Replace all colors</label>`,
			`<label for="mirroring"><input type="checkbox" name="mirroring"> Mirroring</label>`,
			`<button exportFrame>Export frame as image`,
			`<a download="file.txt">`,
			`<button tinyExport>Export .fur`,
			`<p>Import .fur: </p>`,
			`<input tiny type="file">`
		]);

	q(/div[frameNavigation]/)
		.q([`<button firstFrame>⏮`,
			`<button previousFrame disabled>ᐊ`,
			`<p>0/0`,
			`<button nextFrame>ᐅ`,
			`<button lastFrame>⏭`
		]);

	q(/[animation]/).onchange = (evt) => {
		animator.animationIndex = +evt.target.value;
		animator.frameIndex = 0;
		refreshNavigation();
		q(/[animation]/).value = animator.animationIndex;
		animator.draw();
	};

	q(/[createAnimation]/).onclick = () => {
		animator.animations.push({name: "Unnamed animation " + (animator.animations.length + 1), loop: false, FPS: 1, frames: [LED_MATRIX.map(x => [...x])]});
		q(/[animation]/).Q(animator.animations.map((x, i) => `<option value="${i}">${x.name}`));
		q(/[deleteAnimation]/).removeAttribute("disabled");

		animator.animationIndex = animator.animations.length - 1;
		animator.draw();
		q(/[moveAnimationDown]/).removeAttribute("disabled");

		renameAnimations();
	};

	q(/[moveAnimationUp]/).onclick = () => {
		let tmp = animator.animations[animator.animationIndex - 1];
		animator.animations[animator.animationIndex - 1] = animator.animations[animator.animationIndex];
		animator.animationIndex--;
		animator.animations[animator.animationIndex + 1] = tmp;

		q(/[animation]/).Q(animator.animations.map((x, i) => `<option value="${i}">${x.name}`));
		q(/[animation]/).value = animator.animationIndex;

		if (animator.animationIndex < 1)
			q(/[moveAnimationUp]/).setAttribute("disabled", "");
		else
			q(/[moveAnimationUp]/).removeAttribute("disabled");
		if (animator.animationIndex > animator.animations.length - 2)
			q(/[moveAnimationDown]/).setAttribute("disabled", "");
		else
			q(/[moveAnimationDown]/).removeAttribute("disabled");

		animator.draw();
		renameAnimations();
	}

	q(/[moveAnimationDown]/).onclick = () => {
		let tmp = animator.animations[animator.animationIndex + 1];
		animator.animations[animator.animationIndex + 1] = animator.animations[animator.animationIndex];
		animator.animationIndex++;
		animator.animations[animator.animationIndex - 1] = tmp;

		q(/[animation]/).Q(animator.animations.map((x, i) => `<option value="${i}">${x.name}`));
		q(/[animation]/).value = animator.animationIndex;

		if (animator.animationIndex > animator.animations.length - 2)
			q(/[moveAnimationDown]/).setAttribute("disabled", "");
		else
			q(/[moveAnimationDown]/).removeAttribute("disabled");
		if (animator.animationIndex < 1)
			q(/[moveAnimationUp]/).setAttribute("disabled", "");
		else
			q(/[moveAnimationUp]/).removeAttribute("disabled");

		animator.draw();
		renameAnimations();
	}

	q(/[deleteAnimation]/).onclick = () => {
		if (animator.animationIndex < defaultAnimationNames.length) {
			showError(`Can't delete default animation`);
		} else {
			let tmpIndex = animator.animationIndex;
			animator.animationIndex = (animator.animationIndex + 1 === animator.animations.length) ? animator.animationIndex - 1 : animator.animationIndex;
			animator.animations.splice(tmpIndex, 1);
			renameAnimations();
		}
		animator.draw();
	};

	q(/[frameSlider]/).oninput = (evt) => {
		animator.frameIndex = +evt.target.value;
		animator.draw();
		refreshNavigation();
	};

	q(/[previewStart]/).onclick = animator.startPreview.bind(animator);

	q(/[previewStop]/).onclick = animator.stopPreview.bind(animator);

	q(/[firstFrame]/).onclick = () => {
		animator.frameIndex = 0;
		animator.draw();
		refreshNavigation();
	}

	q(/[previousFrame]/).onclick = () => {
		animator.frameIndex--;
		animator.draw();
		refreshNavigation();
	};

	q(/[nextFrame]/).onclick = () => {
		animator.frameIndex++;
		animator.draw();
		refreshNavigation();
	};

	q(/[lastFrame]/).onclick = () => {
		animator.frameIndex = animator.animations[animator.animationIndex].frames.length - 1;
		animator.draw();
		refreshNavigation();
	}

	q(/[showWiring]/).onclick = () => {
		animator.drawWiring = q(/input[name="showWiring"]/).checked;
		animator.draw();
	}

	q(/[animationFPS]/).addEventListener("focusout", changeFPS);
	q(/[animationFPS]/).addEventListener("keydown", e => {
		if (e.keyCode === 13)
			changeFPS();
	});
	q(/[changeFPS]/).onclick = changeFPS;

	q(/[addNextFrame]/).onclick = () => {
		if (animator.animations[animator.animationIndex].frames.length < 2**16 - 1) {
			if (animator.frameIndex + 1 === animator.animations[animator.animationIndex].frames.length)
				animator.animations[animator.animationIndex].frames.push(LED_MATRIX.map(x => [...x]))
			else {
				let frames = animator.animations[animator.animationIndex].frames;
				animator.animations[animator.animationIndex].frames = [...frames.slice(0, animator.frameIndex + 1), LED_MATRIX.map(x => [...x]), ...frames.slice(animator.frameIndex + 1, frames.length)]
			}
			animator.frameIndex++;
			animator.draw();
			refreshNavigation();
		} else {
			showError(`Maximal length of animation is ${2**16 - 1} frames`);
			throw `Maximal length of animation is ${2**16 - 1} frames`;
		}
	};

	q(/[addStartFrame]/).onclick = () => {
		if (animator.animations[animator.animationIndex].frames.length < 2**16 - 1) {
			animator.animations[animator.animationIndex].frames = [LED_MATRIX.map(x => [...x]), ...animator.animations[animator.animationIndex].frames];
			animator.draw();
			refreshNavigation();
		} else {
			showError(`Maximal length of animation is ${2**16 - 1} frames`);
			throw `Maximal length of animation is ${2**16 - 1} frames`;
		}
	};

	q(/[clearFrame]/).onclick = () => {
		animator.animations[animator.animationIndex].frames[animator.frameIndex] = LED_MATRIX.map(x => [...x]);
		animator.draw();
	};

	q(/[copyPreviousFrame]/).onclick = () => {
		animator.animations[animator.animationIndex].frames[animator.frameIndex] = animator.animations[animator.animationIndex].frames[animator.frameIndex - 1].map(x => [...x]);
		animator.draw();
	}

	q(/[importImage]/).onclick = () => q(/[imageImportBackground]/).style.display = "block";

	q(/[importGIF]/).onclick = () => q(/[gifImportBackground]/).style.display = "block";

	q(/[copyFrame]/).onclick = () => {
		frameClipboard = animator.animations[animator.animationIndex].frames[animator.frameIndex].map(x => [...x]);
		q(/[pasteFrame]/).removeAttribute("disabled");
	};

	q(/[copyAnimation]/).onclick = () => {
		animationClipboard = animator.animations[animator.animationIndex].frames.map(x => [...x.map(x => [...x])]);
		q(/[pasteAnimation]/).removeAttribute("disabled");
	};

	q(/[duplicateFrame]/).onclick = () => {
		let copy = q(/[copyFrame]/).onclick;
		let nextFrame = q(/[addNextFrame]/).onclick;
		let paste = q(/[pasteFrame]/).onclick;
		const count = +q(/[duplicateCount]/).value;

		if (count+"" === "NaN" && typeof count === "number" || !count) {
			q(/[duplicateCount]/).value = 1;
			return;
		}

		copy();
		for (var i = 0; i < count; i++) {
			nextFrame();
			paste();
		}
	}

	q(/[pasteFrame]/).onclick = () => {
		animator.animations[animator.animationIndex].frames[animator.frameIndex] = frameClipboard.map(x => [...x])
		animator.draw();
	};

	q(/[pasteAnimation]/).onclick = () => {
		let frames = animator.animations[animator.animationIndex].frames;
		animator.animations[animator.animationIndex].frames = [
			...frames.slice(0, animator.frameIndex + 1),
			...animationClipboard.map(x => [...x.map(x => [...x])]),
			...frames.slice(animator.frameIndex + 1, frames.length)
		];
		animator.frameIndex += animationClipboard.length;
		animator.draw();
		refreshNavigation();
	}

	q(/[deleteFrame]/).onclick = () => {
		let tmpIndex = animator.frameIndex;
		animator.frameIndex = (animator.frameIndex + 1 === animator.animations[animator.animationIndex].frames.length) ? animator.frameIndex - 1 : animator.frameIndex;
		animator.animations[animator.animationIndex].frames.splice(tmpIndex, 1);
		animator.draw();
		refreshNavigation();
	};

	q(/button[tinyExport]/).onclick = exportTinyFile;

	q(/input[tiny][type="file"]/).onchange = importTinyFile;

	q(/[imageImport] [close]/).onclick = () => {
		q(/[imageImportBackground]/).style.display = "none";
		q(/[imageImport] [ok]/).setAttribute("disabled", "");
	}

	q(/[imageImport] input[type="file"]/).onchange = (x) => {
		if (x.target.files[0])
			q(/[imageImport] [ok]/).removeAttribute("disabled");
	}

	q(/[imageImport] [ok]/).onclick = () => {
		let img = new Image();
		img.src = URL.createObjectURL(q(/[imageImport] input[type="file"]/).files[0]);
		img.onload = () => {
			try {
				let ctx = q("<canvas>").getContext("2d");
				ctx.imageSmoothingEnabled = false;
				ctx.canvas.width = MATRIX.width;
				let h, w;
				if (q(/[imageImport] select/).value === "fit") {
					ctx.canvas.height = MATRIX.height - MATRIX.hole.height;
					if(img.width/img.height <= ctx.canvas.width/ctx.canvas.height && q(/[imageImport] select/).value === "fit") {
						h = ctx.canvas.height;
						w = ctx.canvas.height*(img.width/img.height);
					} else {
						h = ctx.canvas.width*(img.height/img.width);
						w = ctx.canvas.width;
					}
				} else {
					ctx.canvas.height = MATRIX.height;
					h = ctx.canvas.width*(img.height/img.width);
					w = ctx.canvas.width;
				}
				ctx.drawImage(img, (ctx.canvas.width - w)/2, (ctx.canvas.height - h)/2, w, h);
				pushFrame(ctx);
				q(/[imageImport] [close]/).click();
			} catch(e) {
				showError("Error importing image");
				console.error(e);
			}
		}
	}

	q(/[gifImport] [close]/).onclick = () => {
		q(/[gifImportBackground]/).style.display = "none";
		q(/[gifImport] [ok]/).setAttribute("disabled", "");
	}

	q(/[gifImport] input[type="file"]/).onchange = async (x) => {
		try {
			if (x.target.files[0]);
				await previewGIF(x.target.files[0], q(/[gifImport] canvas/));
				q(/[gifImport] [ok]/).removeAttribute("disabled");
		} catch (e) {
			showError("Error importing gif");
			console.error(e);
		}
	}

	q(/[gifImport] select/).onchange = async () => {
		try {
			let inputFile = q(/[gifImport] input[type="file"]/).files[0];
			if (inputFile);
				await previewGIF(inputFile, q(/[gifImport] canvas/));
				q(/[gifImport] [ok]/).removeAttribute("disabled");
		} catch (e) {
			showError("Error importing gif");
			console.error(e);
		}
	}

	q(/[gifImport] [ok]/).onclick = async () => {
		try {
			let inputFile = q(/[gifImport] input[type="file"]/).files[0];
			if (inputFile);
				await importGIF(inputFile, q(/[gifImport] canvas/));
				q(/[gifImport] [ok]/).removeAttribute("disabled");
			q(/[gifImport] [close]/).click();
		} catch (e) {
			showError("Error importing gif");
			console.error(e);
		}
	}

	q(/[exportFrame]/).onclick = () => {
		let imageCanvas = q(`<canvas width="${MATRIX.width}" height="${MATRIX.height}">`);
		let imageCtx = imageCanvas.getContext("2d");
		let imageData = imageCtx.createImageData(MATRIX.width, MATRIX.height);
		let frame = animator.animations[animator.animationIndex].frames[animator.frameIndex];
		let data = [];

		for (var y = 0; y < frame.length; y++) {
			for (var x = 0; x < frame[y].length; x++) {
				var pixel = frame[y][x];

				// push RGB values
				pixel.map(byte => data.push(byte));

				// make black pixels transparent
				if (pixel.every(p => p === 0))
					data.push(0);
				else
					data.push(255);
			}
		}

		imageData.data.set(new Uint8ClampedArray(data));
		imageCtx.putImageData(imageData, 0, 0);

		let imageDownload = document.createElement("a");
		imageDownload.download = "frame.png";
		imageDownload.href = imageCanvas.toDataURL("image/png");
		imageDownload.click();
	}

	q(/[importScrollingText]/).onclick = () =>
		q(/[scrollingTextImportBackground]/).style.display = "block";

	q(/[scrollingTextImport] [close]/).onclick = () => {
		q(/[scrollingTextImportBackground]/).style.display = "none";
		q(/[scrollingTextImport] [ok]/).setAttribute("disabled", "");
	}

	q(/[scrollingTextImport] input[type="file"]/).onchange = (x) => {
		if (x.target.files[0])
			q(/[scrollingTextImport] [ok]/).removeAttribute("disabled");
	}

	q(/[scrollingTextImport] [ok]/).onclick = () => {
		let img = new Image();
		img.src = URL.createObjectURL(q(/[scrollingTextImport] input[type="file"]/).files[0]);
		img.onload = () => {
			let canvas = q("<canvas>");
			canvas.height = MATRIX.height;
			canvas.width = MATRIX.width;
			let ctx = canvas.getContext("2d");
			let imageCanvas = q("<canvas>");
			imageCanvas.height = canvas.height - MATRIX.hole.height;
			imageCanvas.width = imageCanvas.height*(img.width/img.height);
			
			let imageCtx = imageCanvas.getContext("2d");

			imageCtx.imageSmoothingEnabled = false;
			imageCtx.drawImage(img, 0, 0, img.width, img.height, 0, 0, imageCanvas.width, imageCanvas.height);

			var offset = 0;
			let startOffset = -canvas.width;

			for (var offset = 0; offset < canvas.width + imageCanvas.width; offset++) {
				ctx.clearRect(0, 0, canvas.width, canvas.height);
				ctx.drawImage(imageCanvas, startOffset + offset, 0, canvas.width, imageCanvas.height, 0, MATRIX.hole.height, canvas.width, imageCanvas.height);
				pushFrame(ctx);
			}

			q(/[scrollingTextImport] [close]/).click()
			refreshNavigation();
		}
	}

	q(/[blur]/).onclick = () => {
		q(/[blurBackground]/).style.display = "block";
		q(/[blurScalingText]/).innerText = `Blur scaling: ${+q(/[blurScaling]/).value}x`;
		q(/[blurAmountText]/).innerText = `Blur amount: ${+q(/[blurAmount]/).value}px`;
		blurPreview.setFrame(animator.animations[animator.animationIndex].frames[animator.frameIndex]);
	}

	q(/[blurScaling]/).oninput = () => {
		q(/[blurScalingText]/).innerText = `Blur scaling: ${+q(/[blurScaling]/).value}x`;
		blurPreview.update();
	}

	q(/[blurAmount]/).oninput = () => {
		q(/[blurAmountText]/).innerText = `Blur amount: ${+q(/[blurAmount]/).value}px`;
		blurPreview.update();
	}

	q(/[blurModal] [close]/).onclick = () => {
		q(/[blurBackground]/).style.display = "none";
	}

	q(/[blurModal] [ok]/).onclick = () => {
		animator.animations[animator.animationIndex].frames = blurPreview.blurFrames(animator.animations[animator.animationIndex].frames);
		animator.draw();
		q(/[blurBackground]/).style.display = "none";
	}

	q(/[fade]/).onclick = () => {
		q(/[fadeBackground]/).style.display = "block";
		fadePreview.setFrame(animator.animations[animator.animationIndex].frames[animator.frameIndex]);
	}

	q(/[fadeModal] [fadeFrameSlider]/).oninput = x => {
		fadePreview.frameIndex = x.target.value - 1;
		fadePreview.draw();
	}

	q(/[fadeModal] [fadeType]/).oninput = () => {
		fadePreview.update();
	}

	q(/[fadeModal] [fadeLength]/).onchange = x => {
		const fadeLength = +q(/[fadeModal] [fadeLength]/).value;
		if (!fadeLength || fadeLength < 1) {
			q(/[fadeModal] [fadeLength]/).value = fadePreview.fadedFrames.length - 1;
		} else {
			fadePreview.update();
		}
	}

	q(/[fadeModal] [close]/).onclick = () => {
		q(/[fadeBackground]/).style.display = "none";
	}

	q(/[fadeModal] [ok]/).onclick = () => {
		const frames = animator.animations[animator.animationIndex].frames;
		animator.animations[animator.animationIndex].frames = [
			...frames.slice(0, animator.frameIndex),
			...fadePreview.fadeFrames(),
			...frames.slice(animator.frameIndex + 1, frames.length)
		];
		animator.draw();
		refreshNavigation();
		q(/[fadeBackground]/).style.display = "none";
	}
}


function refreshNavigation() {
	let animation = animator.animations[animator.animationIndex];
	const FPS = animation.FPS;

	q(/[animation]/).Q(animator.animations.map((x, i) => `<option value="${i}">${x.name}`));
	q(/[animation]/).value = animator.animationIndex;
	q(/[frameSlider]/).max = animation.frames.length - 1;
	q(/[frameSlider]/).value = animator.frameIndex;
	q(/[animationFPS]/).value = animation.FPS;
	q(/[frameNavigation] p/).textContent = (animator.frameIndex + 1) + "/" + animation.frames.length;
	q(/p[timeCode]/).textContent = timeCode(animator.frameIndex, FPS) + "/" + timeCode(animation.frames.length - 1, FPS);

	[/[copyPreviousFrame]/, /[previousFrame]/].map(x => {
		if (animator.frameIndex > 0)
			q(x).removeAttribute("disabled");
		else
			q(x).setAttribute("disabled", "");
	});

	if (animation.frames.length > 1)
		q(/[deleteFrame]/).removeAttribute("disabled");
	else
		q(/[deleteFrame]/).setAttribute("disabled", "");

	if (animator.frameIndex + 1 < animation.frames.length)
		q(/[nextFrame]/).removeAttribute("disabled");
	else
		q(/[nextFrame]/).setAttribute("disabled", "");

	if (animator.animations.length > 1)
		q(/[deleteAnimation]/).removeAttribute("disabled");
	else
		q(/[deleteAnimation]/).setAttribute("disabled", "");

	if (animator.animationIndex < 1)
		q(/[moveAnimationUp]/).setAttribute("disabled", "");
	else
		q(/[moveAnimationUp]/).removeAttribute("disabled");

	if (animator.animationIndex > animator.animations.length - 2)
		q(/[moveAnimationDown]/).setAttribute("disabled", "");
	else
		q(/[moveAnimationDown]/).removeAttribute("disabled");

	if(!frameClipboard)
		q(/[pasteFrame]/).setAttribute("disabled", "");
	else
		q(/[pasteFrame]/).removeAttribute("disabled");
}

function timeCode(frames, fps=1) {
	let total = frames/fps;
	const seconds = (total%60).toFixed(2).padStart(5, 0);
	total -= total%60;
	total /= 60;
	const minutes = (total%60 + "").padStart(2, 0);
	total -= total%60;
	total /= 60;
	const hours = (total + "").padStart(2, 0);

	return `${hours}:${minutes}:${seconds}`;
}

function renameAnimations() {
	for(let i = 0; i < animator.animations.length; i++) {
		animator.animations[i].name = i < defaultAnimationNames.length ?
			defaultAnimationNames[i] :
			"Command animation " + (i + 1 - defaultAnimationNames.length);
	}
	refreshNavigation();
}

function createPreview() {
	q(/#previewHolder/)
		.q([
			`<label for="loopPreview"><input type="checkbox" name="loopPreview"> Loop preview</label>`,
			`<button playPreview>Play preview`,
		]);

	q(/[playPreview]/).onclick = () => (preview.play = true);
}

function compareColors(a, b) {
	return a.every((x, i) => x === b[i]);
}

function replaceColor(oldColor, newColor, MATRIX) {
	return MATRIX.map(row => row.map(color => compareColors(oldColor, color) ? newColor : color));
}

function pushFrame(ctx) {
	q(/[addNextFrame]/).click();
	for (let y = 0; y < ctx.canvas.height; y++) {
		for (let x = 0; x < ctx.canvas.width; x++) {
			let imageData = ctx.getImageData(ctx.canvas.width - x - 1, ctx.canvas.height - y - 1, 1, 1).data;
			animator.animations[animator.animationIndex].frames[animator.frameIndex][MATRIX.height - y - 1][MATRIX.width - x - 1] =
					imageData[3] > 0 ? [...imageData.slice(0, 3)] : [0, 0, 0];
		}
	}
	animator.draw();
}

function scaleGIF(sd, dd) {  // source dimensions, destination dimensions
	let h, w;
	if(sd.width/sd.height <= dd.width/dd.height && q(/[gifImport] select/).value === "fit") {
		h = dd.height;
		w = dd.height*(sd.width/sd.height);
	} else {
		h = dd.width*(sd.height/sd.width);
		w = dd.width;
	}

	return {width: w, height: h};
}

async function previewGIF(file, canvas) {
	let ctx = canvas.getContext("2d");
	let gif = parseGIF(await file.arrayBuffer());
	let gifFrames = decompressFrames(gif, true);
	let frameIndex = 0;
	let [width, height] = ["width", "height"].map(x => gifFrames[0].dims[x]);
	let animationCanvas = q("<canvas>");
	let frameCanvas = q("<canvas>");
	let frameCtx = frameCanvas.getContext("2d");
	
	animationCanvas.width = width;
	animationCanvas.height = height;
	
	let animationCtx = animationCanvas.getContext("2d");
	[width, height] = ["width", "height"].map(x => scaleGIF(gifFrames[0].dims, canvas)[x]);

	if (interval) {
		clearInterval(interval);
		interval = null;
	}

	var drawFrame = () => {
		frameIndex = frameIndex < gifFrames.length ? frameIndex : 0;
		let frame = gifFrames[frameIndex];

		let [frameWidth, frameHeight] = ["width", "height"].map(x => frame.dims[x]);
		
		frameCanvas.width = frameWidth;
		frameCanvas.height = frameHeight;
		
		// check if we should dispose this frame (always dispose after last frame of animation)
		// more disposal methods are describe here: http://www.matthewflickinger.com/lab/whatsinagif/animation_and_transparency.asp
		if (frameIndex === 0 || gifFrames[frameIndex - 1].disposalType === 2) {
			ctx.clearRect(0, 0, canvas.width, canvas.height);
			frameCtx.clearRect(0, 0, frameCanvas.width, frameCanvas.height);
			animationCtx.clearRect(0, 0, animationCanvas.width, animationCanvas.height);
		}

		// use instead of ImageData constructor that might not be available
		let frameData = frameCtx.createImageData(frameWidth, frameHeight);
		frameData.data.set(frame.patch);
		frameCtx.putImageData(frameData, 0, 0);

		animationCtx.drawImage(frameCanvas, frame.dims.left, frame.dims.top);
		ctx.drawImage(animationCanvas, (canvas.width - width)/2, (canvas.height - height)/2, width, height);

		frameIndex++;
		if (!interval)
			interval = setInterval(() => requestAnimationFrame(drawFrame), frame.delay || 100);
	}

	drawFrame();
}

function changeFPS() {
	const FPS = +q(/[animationFPS]/).value;
	if (FPS < 128 && FPS > 0) {
		animator.animations[animator.animationIndex].FPS = q(/[animationFPS]/).value;
	} else {
		showError("Bad FPS value");
		q(/[animationFPS]/).value = animator.animations[animator.animationIndex].FPS;
	}
	animator.draw();
}

async function importGIF(file) {
	let canvas = q("<canvas>");
	let gif = parseGIF(await file.arrayBuffer());
	let gifFrames = decompressFrames(gif, true);
	let [width, height] = ["width", "height"].map(x => gifFrames[0].dims[x]);
	let animationCanvas = q("<canvas>");
	let frameCanvas = q("<canvas>");
	let frameCtx = frameCanvas.getContext("2d");
	
	animationCanvas.width = width;
	animationCanvas.height = height;
	
	let animationCtx = animationCanvas.getContext("2d");
	
	canvas.width = MATRIX.width;
	canvas.height = MATRIX.height - MATRIX.hole.height;

	if (width/height <= canvas.width/canvas.height && q(/[gifImport] select/).value === "fit") {
		[height, width] = [canvas.height, canvas.height*(width/height)];
	} else {
		if (q(/[gifImport] select/).value === "fill")
			canvas.height += MATRIX.hole.height;
		[height, width] = [canvas.width*(height/width), canvas.width];
	}

	let ctx = canvas.getContext("2d");
	ctx.imageSmoothingEnabled = false;

	for (var frameIndex = 0; frameIndex < gifFrames.length; frameIndex++) {
		let frame = gifFrames[frameIndex];

		let [frameWidth, frameHeight] = ["width", "height"].map(x => frame.dims[x]);
		
		frameCanvas.width = frameWidth;
		frameCanvas.height = frameHeight;
		
		// check if we should dispose this frame (always dispose after last frame of animation)
		// more disposal methods are describe here: http://www.matthewflickinger.com/lab/whatsinagif/animation_and_transparency.asp
		if (frameIndex === 0 || gifFrames[frameIndex - 1].disposalType === 2) {
			ctx.clearRect(0, 0, canvas.width, canvas.height);
			frameCtx.clearRect(0, 0, frameCanvas.width, frameCanvas.height);
			animationCtx.clearRect(0, 0, animationCanvas.width, animationCanvas.height);
		}

		// use instead of ImageData constructor that might not be available
		let frameData = frameCtx.createImageData(frameWidth, frameHeight);
		frameData.data.set(frame.patch);
		frameCtx.putImageData(frameData, 0, 0);

		animationCtx.drawImage(frameCanvas, frame.dims.left, frame.dims.top);
		ctx.drawImage(animationCanvas, (canvas.width - width)/2, (canvas.height - height)/2, width, height);

		pushFrame(ctx);
	}
}

function indexBitWidth(colorTableLength) {
	if (colorTableLength < 3)
		return 1;
	else if (colorTableLength < 5)
		return 2;
	else if (colorTableLength < 17)
		return 4;

	return 8;
}

function exportTinyFile() {
	data = [animator.animations.length];  // bytes of data that will be written to the file (each element represents one byte)
	for(const animation of animator.animations) {
		// get all the colors used in this animation (aka the color table)
		let colors = [...new Set(animation.frames.flat().flat().map(x => x+""))].map(x => x.split(",").map(x => +x));
		let writeColor;
		if (colors.length > 256) {
			// the amount of colors used in this animation is too big to fit in array of colors with 1B index
			data.push(animation.FPS);
			writeColor = (frame, led_coordinates, ledCount) => {
				const [x, y] = led_coordinates[ledCount].map(x => x - 1);	// Zero-base coordinate
				frame[y][x].map(x => data.push(x));		// Look up LED's position in frame[][] and push its color
				return 1;  // return the number of colors written
			};
		} else {  // yay, we can use color table for this animation
			data.push(+animation.FPS + 128);  // set the MSB of FPS byte high to indicate that this animation is compressed
			data.push(colors.length - 1);  // we need to know how many colors there are (this number is basically the index of the last color)
			
			let bitWidth = indexBitWidth(colors.length);
			let colorIndex = {};
			colors.map((x, i) => {
				x.map(color => data.push(color));
				colorIndex[x] = i;
			});

			writeColor = (frame, led_coordinates, ledCount) => {
				var byte = 0;  // byte containing index(es)
				for (let i = 0; i < 8/bitWidth; i++) {  // fill the byte with indexes from LSB to MSB
					var index = 0;
					if (ledCount < led_coordinates.length) {
						let [x, y] = led_coordinates[ledCount++].map(x => x - 1);
						index = colorIndex[frame[y][x]];
					}
					byte |= index << (bitWidth*i);
				}

				data.push(byte);
				return 8/bitWidth;  // return the number of indexes written
			}
		}
		// support up to 65535 frames
		data.push(animation.frames.length >> 8); // high byte first
		data.push(animation.frames.length & 0xFF)
		for(const frame of animation.frames) {
			var ledCount = 0;  // how many colors of LEDs we've already written
			while(ledCount < led_coordinates.length) {
				ledCount += writeColor(frame, led_coordinates, ledCount);
			}
		}
	}
	q(/a[download]/).download = "file.fur";
	q(/a[download]/).href = URL.createObjectURL(
		new Blob([new Uint8Array(data)], {type: "application/octet-stream"})
	);
	q(/a[download]/).click();
}

async function importTinyFile(evt) {
	if (evt.target.files[0].name.slice(-4) !== ".fur") {
		showError("Invalid file extension");
		return;
	}

	let bytes = await evt.target.files[0].arrayBuffer().then(x => new Uint8Array(x));
	let importedAnimations = [];
	var animation_offset = 1;  // we can skip this first byte
	for (let i = 0; i < bytes[0]; i++) {
		// animation offset is number of bytes that are before start of this animation data
		let animation = {};
		animation.FPS = bytes[animation_offset++];
		animation.frames = [];

		let getColor;
		if (animation.FPS > 127) {  // MSB of FPS byte indicates that this animation is compressed
			animation.FPS -= 128;  // remove compression indicator from FPS
			let color_table_length = bytes[animation_offset++] + 1;  // how many colors are in the color table
			let color_table = Array(color_table_length);
			for (var j = 0; j < color_table_length; j++) {
				color_table[j] = Array(3);
				for (var k = 0; k < 3; k++) {
					color_table[j][k] = bytes[animation_offset++];
				}
			}

			let indexOffset = 0;  // number of indexes that have been read from the index byte before
			let bitWidth = indexBitWidth(color_table_length);
			getColor = () => {
				let index = (bytes[animation_offset] >> (bitWidth*indexOffset)) & (2**bitWidth - 1);
				indexOffset++;
				if (indexOffset === 8/bitWidth) {  // we read all indexes from the byte
					indexOffset = 0;
					animation_offset++;
				}
				return color_table[index];
			};
		} else 
			getColor = () => [...[0, 0, 0].map((x) => +bytes[animation_offset++])];

		console.log(led_coordinates.length);

		// how many frames are in the animation
		let frame_count = bytes[animation_offset++] << 8 | bytes[animation_offset++];
		for (var j = 0; j < frame_count; j++) {
			let frame = LED_MATRIX.map(x => [...x]);
			for(var k = 0; k < led_coordinates.length; k++) {
				let [x, y] = led_coordinates[k].map(x => x - 1);
				frame[y][x] = getColor();
			}
			animation.frames.push(frame);
		}

		if (!animation.frames.length)  // if fail don't fail
			animation.frames.push(LED_MATRIX.map(x => [...x]))

		if (i < defaultAnimationNames.length) {
			animation.name = defaultAnimationNames[i];
		} else {
			animation.name = "Animation " + (i + 1);
		}
		importedAnimations.push(animation);
	}
	animator.animationIndex = 0;
	animator.frameIndex = 0;
	animator.animations = importedAnimations;

	animator.draw();
	renameAnimations();
	refreshNavigation();
}

function showError(error) {
	q(/[errorBackground]/).style.display = "flex";
	q(/[errorMessage] output/).textContent = error;
}
