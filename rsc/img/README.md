## IMAGES CREATION

Dimension: 320 x 170  

* Export from GIMP as PNG.  
* Apply [RGB565 Image Converter](https://longfangsong.github.io/en/image-to-rgb565/)
* Apply [Binary File to C Array](https://notisrac.github.io/FileToCArray/)
	(Note: This generates a byte array even that we rather would need a int16 array. This is worked-around
	by casting to `(const uint16_t*)MinerScreen` with TFT_eSprite::pushImage() which works since
	ESP32 is little-endian!!!)
* Use [RGB565 Color Picker](https://rgbcolorpicker.com/565                                                                                                                                                                                                                                                                                         ) for RGB to RGB565 convertion

### Font Conversion

 * Using `Orbitron-VariableFont_wght.ttf`from `Orbitron.zip` to convert with [WebFont Generator](https://www.fontsquirrel.com/tools/webfont-generator)  
  (TrueType, Character Type:	Lowercase Uppercase Numbers Punctuation) to generate `orbitron-reduced-glyphs.ttf` and hex array convert with [Binary File to C Array](https://notisrac.github.io/FileToCArray/)
