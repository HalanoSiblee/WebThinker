# WebThinker
<img src="./.media/brand" width="25%" alt="Web Thinker mascot">
Mind mapping graph written in C++ / fltk,sqlite,zstd

# Build
I've Tested only on arch linux
packages :

- fltk 1.4.5 ++
- zstd 1.5.7 ++
- sqlite 3.53.4 ++

```sh
cd Code
make -j$(nproc)
```

# Features
What makes Web thinker better choice mind mapping app than alternative
It's minimalism no bloatware opens instantly total binary size under ~ 140K

- low memory usage
- small size
- small project size file
- massive viewpoint /w snappy performance
- rectangle nodes vectors (square Z-index stacking)
- customization meta for each object

<img src="./.media/optmeter" width="50%" alt="Dashboard Preview">

# Hotkeys

|Key|Action|
|----|---|
|CTRL+LMB|add node|
|RMB|un/link node|
|SHIFT+RMB|Multiselection|
|SHIFT+R|Box selection|
|SHIFT+LMB|Pan|
|ALT+LMB|Drag|
|S|add square|
|F|find/search|
|V|add vector|
|Del|delete|
|F1-F4|fast edit|

---
![WebThinker](./.media/img1)
![DarkTriad](./.media/img2)

---
___Project structuring___      : [@HalanoSiblee](https://github.com/HalanoSiblee)\
Codder Model             : [@XAi.Grok](http://grok.com/)\
Codder environment       : Ubuntu 24.04.4 LTS\
Author finalization env  : Arch Linux