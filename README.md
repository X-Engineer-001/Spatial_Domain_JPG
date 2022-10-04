獨特的圖片壓縮思維跟研究方向。<br>
<br>
GitHub放不下超大的.bmp圖檔......[Google Drive](https://drive.google.com/drive/folders/1RqmBng2F007P9GvN6ec18qIu3THddPA2?usp=sharing)<br>
執行檔使用static linking編譯僅為提高portability。<br>
<br>
有趣的小插曲：<br>
為了推甄在整理這些東西，要把decompressor重新編譯成static linking的時候，忽然出現編譯錯誤！原來是當時寫的絕對值函數忘了拿掉，其實後來也沒有用到，本來留著也無傷大雅。但是當時舊電腦裝的是C++ 11或13，本來std是沒有double型態的abs()的，結果換了新電腦，現在都到C++ 23了，就跟std裡面新加的abs衝突了XD<br>
