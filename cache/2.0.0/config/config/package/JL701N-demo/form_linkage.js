
/**
  环境变量
    src:当前操作的控件
    find函数：查找当前页面的空间 参数：控件名称 xxx.xxx
    show函数：消息提醒  参数：text 提示文本
    
  控件属性
    name:控件的名称：xxx.xxx 只读
  disabled：是否禁止控件
  min：控件最小值：计数器型控件有效
  max：控件最大值：计数器型控件有效
  val：控件值
  options：下拉选择类控件有效,数组类型，子属性 ：value、label、disable、hide
  groupEnable:分组右上角的状态
  isUndo: 判断是否在执行撤销操作(只在src中可读)
  undoVal: 记录修改前的值，在src.isUndo为true时，可以修改groupEnable/val 为这个值
*/
/*
// 强关联项的直接写正反向操作即可正常撤销，如：if(a.val==1)b.val=1;if(a.val==0)b.val=0
// 弱关联项撤销例子： MSBC例子，在修改dest.val之前把旧val记录在dest.undoVal上,
if (src.name == '经典蓝牙.MSBC') {
  const dest = find('$板级配置.编码配置.MSBC')
  let undoVal = []
  if (dest.undoVal && Array.isArray(dest.undoVal)) undoVal = dest.undoVal

  if (src.isUndo) {//撤销操作
    dest.val = undoVal.pop()
  } else {
    undoVal.push(dest.val)
    dest.undoVal = undoVal

    //非撤销操作再判断脚本操作
    if (src.val == 1) {
      dest.val = 1;
    }
  }

}
//开关例子： 电源配置,时钟&电源.电源模式 受 时钟&电源.powerDown控制
if(src.name == '时钟&电源.powerDown'){
  const dest =  find('时钟&电源.电源模式');	
  if(!dest){
    return
  }
  dest.disabled=src.val;
	
}
// 下拉选择例子： 涓流电流 约束恒流电流
else if(src.name == '充电配置.涓流电流'){
  const dest =  find('充电配置.恒流电流');
    if(!dest){
    return
  }
  if(src.val==='30mA'){
    //选项禁止选择
    dest.options[1].disabled = true;
    //选项不显示
    dest.options[2].hide = false;
    // 修改值
    if(dest.val == '40mA' || dest.val =='30mA'){
      dest.val = '50mA'
    }
  }else {
    dest.options[1].disabled = false;
    dest.options[2].hide = true; 
  }	
}

//计数器例子
else if(src.name == '充电配置.开机充电'){
  const dest =  find('充电配置.入舱滤波时间(ms)');
	
  if(!dest){
    return
  }
  if(src.val){
    dest.min = 0;
    dest.max = 600;
  }else{
    dest.min = 10;
    dest.max = 65535;
  }	
  // 纠正值
  if(dest.val > dest.max){
    dest.val = dest.max;
  }else if(dest.val < dest.min){
    dest.val = dest.min;
  }
	
}

// 分组使能
else if(src.name =='充电配置'){
  const dest = find('智能仓');
  dest.groupEnable = !src.groupEnable;
}

// 消息提示

else if(src.name =='充电配置.入舱滤波时间(ms)'){
  if(src.val > 20){
    src.val = 20
    show('最大值不能超过20')
  }
}

else if (src.name == '内置触摸按键配置') {
  if (src.groupEnable == true) {
    const dest = find('内置触摸按键配置.键值') 
    if (dest) {
      dest.val = 'KEY_SLIDER'
    }
  }
	
}
*/

if (src.name == '第0段.Detect Mode') {
  const dest = find('第0段.Rms Time')
  if (dest) {
    if (src.val == 'PEAK') {
      dest.disabled = true;
    } else {
      dest.disabled = false;
    }
  }
}
if (src.name == '第0段.Effect Mode') {
  const dest = find('第0段.Max Enhance or Decay Gain')
  const dest2 = find('第0段.Ratio')
  if (dest) {
    if (src.val == 'ENHANCE') {
      dest.src.__config__.tip = "范围（0~30）dB"
      dest.min = 0;
      dest.max = 30;
      dest.val = 30;
    } else {
      dest.src.__config__.tip = "范围（-30~0）dB"
      dest.min = -30;
      dest.max = 0;
      dest.val = -30;
    }
  }
  if (dest2) {
    if (src.val == 'ENHANCE') {
      dest2.src.__config__.tip = "范围0.1~1"
      dest2.min = 0.1;
      dest2.max = 1;
      dest2.val = 1;
    } else {
      dest2.src.__config__.tip = "范围1~30"
      dest2.min = 1;
      dest2.max = 30;
      dest2.val = 30;
    }
  }
}
if (src.name == '第0段.Threshold') {
  const dest = find('第0段.Noisegate Threshold', true)
  if (dest) {
    if (src.val <= dest.val) {
      src.val = dest.val;
      show('Noisegae Threshold 应当比Threshold 小')
    }
  }
}
if (src.name == '第0段.Noisegate Threshold') {
  const dest = find('第0段.Threshold')
  if (dest) {
    if (src.val >= dest.val) {
      src.val = dest.val;
      show('Noisegae Threshold 应当比Threshold 小')
    }
  }
}

////////////////////////////////
if (src.name == '第1段.Detect Mode') {
  const dest = find('第1段.Rms Time')
  if (dest) {
    if (src.val == 'PEAK') {
      dest.disabled = true;
    } else {
      dest.disabled = false;
    }
  }
}
if (src.name == '第1段.Effect Mode') {
  const dest = find('第1段.Max Enhance or Decay Gain')
  const dest2 = find('第1段.Ratio')
  if (dest) {
    if (src.val == 'ENHANCE') {
      dest.src.__config__.tip = "范围（0~30）dB"
      dest.min = 0;
      dest.max = 30;
      dest.val = 30;
    } else {
      dest.src.__config__.tip = "范围（-30~0）dB"
      dest.min = -30;
      dest.max = 0;
      dest.val = -30;
    }
  }
  if (dest2) {
    if (src.val == 'ENHANCE') {
      dest2.src.__config__.tip = "范围0.1~1"
      dest2.min = 0.1;
      dest2.max = 1;
      dest2.val = 1;
    } else {
      dest2.src.__config__.tip = "范围1~30"
      dest2.min = 1;
      dest2.max = 30;
      dest2.val = 30;
    }
  }
}
if (src.name == '第1段.Threshold') {
  const dest = find('第1段.Noisegate Threshold', true)
  if (dest) {
    if (src.val <= dest.val) {
      src.val = dest.val;
      show('Noisegae Threshold 应当比Threshold 小')
    }
  }
}
if (src.name == '第1段.Noisegate Threshold') {
  const dest = find('第1段.Threshold')
  if (dest) {
    if (src.val >= dest.val) {
      src.val = dest.val;
      show('Noisegae Threshold 应当比Threshold 小')
    }
  }
}

////////////////////////////////
if (src.name == '第2段.Detect Mode') {
  const dest = find('第2段.Rms Time')
  if (dest) {
    if (src.val == 'PEAK') {
      dest.disabled = true;
    } else {
      dest.disabled = false;
    }
  }
}
if (src.name == '第2段.Effect Mode') {
  const dest = find('第2段.Max Enhance or Decay Gain')
  const dest2 = find('第2段.Ratio')
  if (dest) {
    if (src.val == 'ENHANCE') {
      dest.src.__config__.tip = "范围（0~30）dB"
      dest.min = 0;
      dest.max = 30;
      dest.val = 30;
    } else {
      dest.src.__config__.tip = "范围（-30~0）dB"
      dest.min = -30;
      dest.max = 0;
      dest.val = -30;
    }
  }
  if (dest2) {
    if (src.val == 'ENHANCE') {
      dest2.src.__config__.tip = "范围0.1~1"
      dest2.min = 0.1;
      dest2.max = 1;
      dest2.val = 1;
    } else {
      dest2.src.__config__.tip = "范围1~30"
      dest2.min = 1;
      dest2.max = 30;
      dest2.val = 30;
    }
  }
}
if (src.name == '第2段.Threshold') {
  const dest = find('第2段.Noisegate Threshold', true)
  if (dest) {
    if (src.val <= dest.val) {
      src.val = dest.val;
      show('Noisegae Threshold 应当比Threshold 小')
    }
  }
}
if (src.name == '第2段.Noisegate Threshold') {
  const dest = find('第2段.Threshold')
  if (dest) {
    if (src.val >= dest.val) {
      src.val = dest.val;
      show('Noisegae Threshold 应当比Threshold 小')
    }
  }
}

////////////////////////////////
if (src.name == '第3段.Detect mode') {
  const dest = find('第3段.Rms Time')
  if (dest) {
    if (src.val == 'PEAK') {
      dest.disabled = true;
    } else {
      dest.disabled = false;
    }
  }
}
if (src.name == '第3段.Effect Mode') {
  const dest = find('第3段.Max Enhance or Decay Gain')
  const dest2 = find('第3段.Ratio')
  if (dest) {
    if (src.val == 'ENHANCE') {
      dest.src.__config__.tip = "范围（0~30）dB"
      dest.min = 0;
      dest.max = 30;
      dest.val = 30;
    } else {
      dest.src.__config__.tip = "范围（-30~0）dB"
      dest.min = -30;
      dest.max = 0;
      dest.val = -30;
    }
  }
  if (dest2) {
    if (src.val == 'ENHANCE') {
      dest2.src.__config__.tip = "范围0.1~1"
      dest2.min = 0.1;
      dest2.max = 1;
      dest2.val = 1;
    } else {
      dest2.src.__config__.tip = "范围1~30"
      dest2.min = 1;
      dest2.max = 30;
      dest2.val = 30;
    }
  }
}
if (src.name == '第3段.Threshold') {
  const dest = find('第3段.Noisegate Threshold', true)
  if (dest) {
    if (src.val <= dest.val) {
      src.val = dest.val;
      show('Noisegae Threshold 应当比Threshold 小')
    }
  }
}
if (src.name == '第3段.Noisegate Threshold') {
  const dest = find('第3段.Threshold')
  if (dest) {
    if (src.val >= dest.val) {
      src.val = dest.val;
      show('Noisegae Threshold 应当比Threshold 小')
    }
  }
}

/////////////////////////////
//段与段之间算法类型的约束,跟随第0段
if (src.name == '第3段.Detect Mode') {
  const dest = find('第0段.Detect Mode')
  /* if (dest) { */
    // if (src.val != dest.val) {
      // src.val = dest.val;
      // show('算法类型跟随第0段')
    // }
  /* } */
  const dest_t = find('第3段.Rms Time')
  if (dest_t) {
    if (src.val == 'PEAK') {
      dest_t.disabled = true;
    } else {
      dest_t.disabled = false;
    }
  }
}
if (src.name == '第2段.Detect Mode') {
  const dest = find('第0段.Detect Mode')
  /* if (dest) { */
    // if (src.val != dest.val) {
      // src.val = dest.val;
      // show('算法类型跟随第0段')
    // }
  /* } */
  const dest_t = find('第2段.Rms Time')
  if (dest_t) {
    if (src.val == 'PEAK') {
      dest_t.disabled = true;
    } else {
      dest_t.disabled = false;
    }
  }
}
if (src.name == '第1段.Detect Mode') {
  const dest = find('第0段.Detect Mode')
  /* if (dest) { */
    // if (src.val != dest.val) {
      // src.val = dest.val;
      // show('算法类型跟随第0段')
    // }
  /* } */
  const dest_t = find('第1段.Rms Time')
  if (dest_t) {
    if (src.val == 'PEAK') {
      dest_t.disabled = true;
    } else {
      dest_t.disabled = false;
    }
  }
}

if (src.name == '第0段.Detect Mode') {
  const dest1 = find('第1段.Detect Mode')
  const dest2 = find('第2段.Detect Mode')
  const dest3 = find('第3段.Detect Mode')
/*   if (dest1) { */
    // if (dest1.val != src.val) {
      // dest1.val = src.val;
    // }
  // }
  // if (dest2) {
    // if (dest2.val != src.val) {
      // dest2.val = src.val;
    // }
  // }
  // if (dest3) {
    // if (dest3.val != src.val) {
      // dest3.val = src.val;
    // }
  /* } */
  const dest11 = find('第1段.Rms Time')
  const dest22 = find('第2段.Rms Time')
  const dest33 = find('第3段.Rms Time')
  if (dest11) {
    if (dest1.val == 'PEAK') {
      dest11.disabled = true;
    } else {
      dest11.disabled = false;
    }
  }
  if (dest22) {
    if (dest2.val == 'PEAK') {
      dest22.disabled = true;
    } else {
      dest22.disabled = false;
    }
  }
  if (dest33) {
    if (dest3.val == 'PEAK') {
      dest33.disabled = true;
    } else {
      dest33.disabled = false;
    }
  }
}

//显示隐藏脚本 status: 0 :显示 1：仅隐藏（不影响导出） 2:隐藏且不导出
if (src.name == 'Dynamic EQ.nSection') {
  let dest0 = find('第0段')
  let dest1 = find('第1段')
  let dest2 = find('第2段')
  let dest3 = find('第3段')

  if (src.val == 1) {
    if (dest0) {
      dest0.status = 0;
    }
    if (dest1) {
      dest1.status = 1;
    }
    if (dest2) {
      dest2.status = 1;
    }
    if (dest3) {
      dest3.status = 1;
    }
  }
  if (src.val == 2) {
    if (dest0) {
      dest0.status = 0;
    }
    if (dest1) {
      dest1.status = 0;
    }
    if (dest2) {
      dest2.status = 1;
    }
    if (dest3) {
      dest3.status = 1;
    }
  }
  if (src.val == 3) {
    if (dest0) {
      dest0.status = 0;
    }
    if (dest1) {
      dest1.status = 0;
    }
    if (dest2) {
      dest2.status = 0;
    }
    if (dest3) {
      dest3.status = 1;
    }
  }
  if (src.val == 4) {
    if (dest0) {
      dest0.status = 0;
    }
    if (dest1) {
      dest1.status = 0;
    }
    if (dest2) {
      dest2.status = 0;
    }
    if (dest3) {
      dest3.status = 0;
    }
  }

}
if (src.name == 'Dynamic EQ Ext Detector.nSection') {
  let dest0 = find('第0段')
  let dest1 = find('第1段')
  let dest2 = find('第2段')
  let dest3 = find('第3段')
  if (src.val == 1) {
    if (dest0) {
      dest0.status = 0;
    }
    if (dest1) {
      dest1.status = 1;
    }
    if (dest2) {
      dest2.status = 1;
    }
    if (dest3) {
      dest3.status = 1;
    }
  }
  if (src.val == 2) {
    if (dest0) {
      dest0.status = 0;
    }
    if (dest1) {
      dest1.status = 0;
    }
    if (dest2) {
      dest2.status = 1;
    }
    if (dest3) {
      dest3.status = 1;
    }
  }
  if (src.val == 3) {
    if (dest0) {
      dest0.status = 0;
    }
    if (dest1) {
      dest1.status = 0;
    }
    if (dest2) {
      dest2.status = 0;
    }
    if (dest3) {
      dest3.status = 1;
    }
  }
  if (src.val == 4) {
    if (dest0) {
      dest0.status = 0;
    }
    if (dest1) {
      dest1.status = 0;
    }
    if (dest2) {
      dest2.status = 0;
    }
    if (dest3) {
      dest3.status = 0;
    }
  }

}

//显示隐藏脚本 status: 0 :显示 1：仅隐藏（不影响导出） 2:隐藏且不导出
if (src.name == 'Spectrum Advance.nSection') {
	
  const maxSection = src.max
  const curSection = src.val
  
  for(let i = 0;i<maxSection;i++){
	  const dest = find("第"+i+"段")
      dest.status = i<curSection?0:1 
  }
}

var pattern = /^第[0-9]{1,2}段.Algorithm Type$/
if (pattern.test(src.name)) {
   // console.error('errr',src)
       const destName = src.name.replace('Algorithm Type','Rms Time')
       const dest = find(destName)
       if (dest){
           if (src.val == 'PEAK') {
               dest.disabled = true;
           } else {
               dest.disabled = false;
           }
       }
}
if (src.name == 'AEC回采配置.回采类型') {
  let dest0 = find('AEC回采配置.硬回采MIC选择')
  if (src.val == '软回采') {
	if (dest0) {
      dest0.status = 1;
    }  
  }
  if (src.val == '硬回采') {
    if (dest0) {
      dest0.status = 0;
    }
  }
}

/////////////////////////////

if (src.name == 'Energy Detect.Mute Energy') {
  const dest = find('Energy Detect.Unmute Energy')
  if (dest) {
    if (src.val >= dest.val) {
      src.val = dest.val;
      show('Mute Energy 应当比Unmute Energy 小')
    }
  }
}
if (src.name == 'Energy Detect.Unmute Energy') {
  const dest = find('Energy Detect.Mute Energy')
  if (dest) {
    if (src.val <= dest.val) {
      src.val = dest.val;
      show('Mute Energy 应当比Unmute Energy 小')
    }
  }
}
if (src.name == 'Energy Detect.Mute Time(ms)') {
  const dest = find('Energy Detect.Count Cycle(ms)')
  if (dest) {
    if (src.val <= dest.val) {
      src.val = dest.val;
      show('Mute Time(ms) 应当比Count Cycle(ms) 大')
    }
  }
}
if (src.name == 'Energy Detect.Count Cycle(ms)') {
  const dest = find('Energy Detect.Mute Time(ms)')
  if (dest) {
    if (src.val >= dest.val) {
      src.val = dest.val;
      show('Mute Time(ms) 应当比Count Cycle(ms) 大')
    }
  }
}
if (src.name == 'CrossOver 3Band.低中分频点') {
  const dest = find('CrossOver 3Band.中高分频点')
  if (dest) {
    if (src.val >= dest.val) {
      src.val = dest.val;
      show('中高分频点应当比低中分频点大')
    }
  }
}
if (src.name == 'CrossOver 3Band.中高分频点') {
  const dest = find('CrossOver 3Band.低中分频点')
  if (dest) {
    if (src.val <= dest.val) {
      src.val = dest.val;
      show('中高分频点应当比低中分频点大')
    }
  }
}


if (src.name == '经典蓝牙.蓝牙后台') {
  const dest = find('经典蓝牙.蓝牙后台连接断开返回');
  dest.val = src.val;
}

if (src.name == '经典蓝牙.蓝牙后台') {
  const dest = find('经典蓝牙.音乐检测时间');
  if (src.val) {
    dest.disabled = false;
  } else {
    dest.disabled = true;
  }

}
if (src.name == '经典蓝牙.蓝牙后台') {
  const dest = find('经典蓝牙.蓝牙后台连接断开返回');
  if (!src.val) {
    dest.val = src.val;
  }
}

if (src.name == 'TWS' && !src.groupEnable) {
  let dest = find("TWS.TWS本地音乐转发")
  if(dest.val) {
    dest.val = 0;
    const mediaFlow = find('$媒体')
    mediaFlow.changeGroup('立体声')  
    show("媒体流程图切换为立体声")
    dest = find('$音频配置.DAC配置.声道配置');
    if (dest) {
        dest.val = '双声道';
    }
  }
}

if (src.name == '经典蓝牙.蓝牙后台' && !src.val) {
	let dest = find('TWS')
	if(dest.groupEnable){
    dest = find('TWS.TWS本地音乐转发')
    if(dest.val) {
      show("打开TWS本地音乐转发必须打开蓝牙后台", true)
      src.val = 1;
      dest = find('经典蓝牙.蓝牙后台连接断开返回');
      dest.val = 1;
    } 
	}
}

if (src.name == "TWS.TWS本地音乐转发") {
  const mediaFlow = find('$媒体')
  const usbFlow = find('$USB Audio')
  if(src.val) {
    let dest = find("$功能配置.APP模式配置.蓝牙模式")
    if(dest.val == 0) {
      show('需要打开蓝牙模式', true)
      src.val = 0;
      return;
    }
    dest = find("TWS")
    if(dest.groupEnable == 0) {
      show("需要先打开TWS功能", true)
      src.val = 0
    } else {
      dest = find("经典蓝牙.蓝牙后台")
      if(!dest.val) {
        dest.val = 1;
      }
      dest = find("$音频配置.编码配置.SBC")
      dest.val = 1;
      dest = find("$音频配置.DAC配置.缓冲长度（ms）")
      dest.val = 100
      dest = find("TWS.本地音乐音量同步")
      dest.val = 1;
      mediaFlow.changeGroup('local_TWS')  
      usbFlow.changeGroup('local_TWS')  	  
      show("媒体和USB Audio流程图切换为local TWS")
    }   
  } else {
    const mediaFlow1 = find('$系统模式')
    show("媒体和USB Audio流程图切换为默认配置")
    mediaFlow.changeGroup(mediaFlow1.currentGroup)  
	usbFlow.changeGroup('默认分组')    
    const edr_conn = find('TWS.后台关闭经典蓝牙连接')
    edr_conn.val = 0;
  }
}

if(src.name == 'TWS.后台关闭经典蓝牙连接' && src.val) {
  const dest = find('TWS.TWS本地音乐转发')
  if(dest.val == 0) {
    src.val = 0;
    show("需要打开本地音乐转发才支持此配置", true);
  }
}

if(src.name == '发送参数配置.编码类型'){
  if(src.val == 'JLA') {
    let dest = find('$音频配置.编码配置.JLA')
    dest.val = 1;
    dest = find('发送参数配置.编码帧数')
    dest.val = 2;
  } 
  if(src.val == 'SBC') {
    let dest = find('$音频配置.编码配置.SBC')
    dest.val = 1;
    dest = find('发送参数配置.编码帧数')
    dest.val = 4;
  }
}

if(src.name == "解码相关配置.自定义解码格式.JLA" && !src.val){
  let dest = find("$蓝牙配置.TWS.TWS本地音乐转发")
  if(dest.val) {
    show("请确认local_TWS流程图中LocalTWS Source节点的解码格式配置是否为JLA格式，如果是JLA格式，请不要关闭此开关",  true)
  }
}

if(src.name == "编码配置.JLA" && !src.val){
  let dest = find("$蓝牙配置.TWS.TWS本地音乐转发")
  if(dest.val) {
    show("请确认local_TWS流程图中LocalTWS Source节点的解码格式配置是否为JLA格式，如果是JLA格式，请不要关闭此开关",  true)
  }
}

if(src.name == "解码相关配置.自定义解码格式.SBC" && !src.val){
  let dest = find("$蓝牙配置.TWS.TWS本地音乐转发")
  if(dest.val) {
    show("请确认local_TWS流程图中LocalTWS Source节点的解码格式配置是否为SBC格式，如果是SBC格式，请不要关闭此开关",  true)
  }
}

if(src.name == "编码配置.SBC" && !src.val){
  let dest = find("$蓝牙配置.TWS.TWS本地音乐转发")
  if(dest.val) {
    show("请确认local_TWS流程图中LocalTWS Source节点的解码格式配置是否为SBC格式，如果是SBC格式，请不要关闭此开关",  true)
  }
}

if(src.name == 'APP模式配置.蓝牙模式' && src.val == 0) {
  let dest = find("$蓝牙配置.TWS")
  dest.groupEnable = 0;
  dest = find("$蓝牙配置.TWS.TWS本地音乐转发")
  dest.val = 0;
}

if (src.name == 'Bass Treble.低音.Cur Gain') {
  const dest1 = find('Bass Treble.低音.Min Gain')
  const dest2 = find('Bass Treble.低音.Max Gain')
  if (dest1 && dest2) {
    src.min = dest1.val;
    src.max = dest2.val;
    if (src.val >= src.max) {
      src.val = src.max;
    }
    if (src.val <= src.min) {
      src.val = src.min;
    }
  }
}
if (src.name == 'Bass Treble.中音.Cur Gain') {
  const dest1 = find('Bass Treble.中音.Min Gain')
  const dest2 = find('Bass Treble.中音.Max Gain')
  if (dest1 && dest2) {
    src.min = dest1.val;
    src.max = dest2.val;
    if (src.val >= src.max) {
      src.val = src.max;
    }
    if (src.val <= src.min) {
      src.val = src.min;
    }
  }
}

if (src.name == 'Bass Treble.高音.Cur Gain') {
  const dest1 = find('Bass Treble.高音.Min Gain')
  const dest2 = find('Bass Treble.高音.Max Gain')
  if (dest1 && dest2) {
    src.min = dest1.val;
    src.max = dest2.val;
    if (src.val >= src.max) {
      src.val = src.max;
    }
    if (src.val <= src.min) {
      src.val = src.min;
    }
  }
}


if (src.name == '经典蓝牙.手机铃声') {
  const dest1 = find('经典蓝牙.来电报号')
  if (src.val && dest1.val) {
    dest1.val = 0;
  }
}

if (src.name == '经典蓝牙.来电报号') {
  const dest1 = find('经典蓝牙.手机铃声')
  if (src.val && dest1.val) {
    dest1.val = 0;
  }
}

if (src.name == '蓝牙模式选择.模式选择') {
  const dest1 = find('蓝牙模式选择.NORMAL模式下使能DUT测试')
  if (src.val != 'NORMAL') {
    dest1.disabled = 1;
    dest1.val = 0;
  } else if (src.val == 'NORMAL') {
    dest1.disabled = 0;
  }
}

if (src.name == '蓝牙模式选择.NORMAL模式下使能DUT测试') {
  if (src.val == 1) {
    show('默认串口通信口为USBDM/DP，需要将PC模式、U盘使能关闭才能正常通信', true);
  }
  const dest1 = find('蓝牙模式选择.模式选择')
  if (dest1.val != 'NORMAL') {
    src.disabled = 1;
  } else {
    src.disabled = 0;
  }
}

if (src.name == '存储配置.flash通信') {
  const dest = find('存储配置.flash容量')
  if (src.val == '单线-1bit') {
    dest.val = '4Mbit'
  } else {
    dest.val = '8Mbit'
  }
}

if (src.name === '流程图.使能控制') {
   const moudle = src.moudle // 流程模块名称
   if(moudle=='pipeline-mic_effect'){	   
	 const dest = find('$功能配置.混响配置.混响使能')
	 dest.val = src.enable?0x01:0x00
   }
}



if (src.name === '混响配置.混响使能') {
   const dest = find('$麦克风音效') // 流程图模块   
   console.error('errr',dest)
   dest.enable = src.val 
   console.error('set enable',dest)
   
}

// 流程图分组切换
if (/^流程图\.分组切换\..+$/.test(src.name)) {
  const moudle = src.moudle // 流程模块名称
  const group = src.name.replace('流程图.分组切换.','')
  let dest = find("$蓝牙配置.TWS.TWS本地音乐转发");

  if(group == 'local_TWS') {
    show('切换媒体流程图到local_TWS会打开TWS本地音乐转发', true)
    dest.val = 1;
    let dest_tws = find('$蓝牙配置.TWS')
    dest_tws.groupEnable = 1;
    dest_tws = find('$蓝牙配置.经典蓝牙.蓝牙后台')
    dest_tws.val = 1;
  }
  
  if(src.moudle == 'pipeline-Media' && dest.val && group != 'local_TWS') {
    show('切换媒体流程图会关掉TWS本地音乐', true)
    dest.val = 0;
  }

  const flowFileNames = listFileNames('flow') // 获取所有流程图的文件名称
  //show(group)
  for(const fileName of flowFileNames) {
    //if (dest.val && fileName == '$媒体') {
    //  continue
    //}
	  const flowFile = find(fileName)  // 获取流程图句柄
	  if(flowFile.hasGroup(group)){  // 判断是否存在分组
		  flowFile.changeGroup(group)
	  } else if(fileName == '$USB Audio'){
		  if(group != 'local_TWS') {
        const MediaFlowFile = find('$媒体') 
        if(MediaFlowFile.currentGroup != 'local_TWS') {
			    flowFile.changeGroup('默认分组')
        }
		  }
	  }	  
  }	 

  dest = find('$音频配置.DAC配置.声道配置');
  if(group == '单声道' || group == '单声道pro') {
    show('DAC声道配置切换到单声道')
    dest.val = '单声道 LP-RP';
  } else if(group == '立体声' || group == '立体声pro') {
    show('DAC声道配置切换到双声道')
    dest.val = '双声道';
  } else if(group == '2.1_channel' || group == '2.1_channel_pro') {
    show('DAC声道配置切换到四声道')
    dest.val = '四声道'
  } else if (group == '1.1_channel') {
	show('DAC声道配置切换到双声道')
    dest.val = '双声道';
  }
}

/*
if (src.name == 'FW编辑、在线调音') {
  const dest = find('$蓝牙配置.sniff')
  if (src.groupEnable && dest.groupEnable) {
    dest.groupEnable = 0;
    show('在线调音打开关闭sniff');
  }
  if (!src.groupEnable && !dest.groupEnable) {
    dest.groupEnable = 1;
    show('在线调音关闭打开sniff');
  }
}
*/

if (src.name == 'sniff') {
  if (src.groupEnable) {
    show('需要到电源配置里打开低功耗模式，系统才可进入低功耗');
  }
  /*
  const dest = find('$板级配置.FW编辑、在线调音')
  if (src.groupEnable && dest.groupEnable) {
    dest.groupEnable = 0;
    show('sniff打开关闭在线调音');
  }
  if (!src.groupEnable && !dest.groupEnable) {
    dest.groupEnable = 1;
    show('sniff关闭打开在线调音');
  }*/
}

//蓝牙HFP限制
if (src.name == '经典蓝牙.HFP') {
  const src1 = find('经典蓝牙.仅保留电量显示')
  if (src1.val) {
    const src2 = find('经典蓝牙.MSBC')
    const dest1 = find('$音频配置.编码配置.MSBC')
    const dest2 = find('$音频配置.编码配置.CVSD')
    if (src.val) {
      dest2.val = 1;
      if (src2.val == 1) {
        dest1.val = 1;
      }
    }
  }
}

if (src.name == '经典蓝牙.LDAC') {
	const dest = find('$音频配置.通用音频配置.全局采样率')
	if (src.val == 1){
		show('全局采样率需要配置为96000Hz');
		dest.val = '96000Hz';
		dest.disabled = 1;
	}else{
		dest.val = '44100Hz';
		dest.disabled = 0;
	}
}

if (src.name == '经典蓝牙.MSBC') {
  const dest = find('$音频配置.编码配置.MSBC')
  let undoVal = []
  if (dest.undoVal && Array.isArray(dest.undoVal)) undoVal = dest.undoVal

  if (src.isUndo) {//撤销操作
    dest.val = undoVal.pop()
  } else {
    undoVal.push(dest.val)
    dest.undoVal = undoVal

    //非撤销操作再判断脚本操作
    if (src.val == 1) {
      dest.val = 1;
    }
  }

}

if (src.name == '编码配置.MSBC') {
  const dest1 = find('$蓝牙配置.经典蓝牙.HFP')
  const dest2 = find('$蓝牙配置.经典蓝牙.MSBC')
  const dest3 = find('$蓝牙配置.经典蓝牙.仅保留电量显示')
  if (!src.val && dest1.val && dest2.val && !dest3.val) {
    src.val = 1;
    show('因蓝牙通话配置不可关闭MSBC编码');
  }
}

if (src.name == '编码配置.CVSD') {
  const dest1 = find('$蓝牙配置.经典蓝牙.HFP')
  const dest2 = find('$蓝牙配置.经典蓝牙.仅保留电量显示')
  if (!src.val && dest1.val && !dest2.val) {
    src.val = 1;
    show('因蓝牙通话配置不可关闭CVSD编码');
  }
}

//蓝牙无连接自动关机功能
if (src.name == '经典蓝牙.无连接自动关机') {
  const dest1 = find('经典蓝牙.无连接关机时间(s)')
  if (src.val == 0) {
    dest1.min = 0;
    dest1.val = dest1.min;
    dest1.disabled = 1;
  } else {
    dest1.min = 60;
    dest1.val = 120;
    dest1.disabled = 0;
  }
}

if (src.name == 'DAC配置.输出方式') {
  const dest2 = find('$电源配置.时钟&电源.IOVDD')  
  var _iovdd_val = dest2.val
  _iovdd_val = parseFloat(_iovdd_val.slice(0,_iovdd_val.length-1)) * 1000;
 
  console.error(dest2)
  if (src.val == '高压一档单端'|| src.val == '高压一档差分') {
    if(_iovdd_val<3000){
      show('IOVDD需要≥3.0V！！！');
    }
  } else if (src.val == '高压二档单端'|| src.val == '高压二档差分') {
    if(_iovdd_val<3300){
      show('IOVDD需要≥3.4V！！！');
    }
  }
}

//----------------------------ADC供电选择逻辑控制------------------------------------------
if (src.name == 'MIC 0 配置.供电端口' || src.name == 'MIC 1 配置.供电端口' 
|| src.name == 'MIC 2 配置.供电端口'|| src.name == 'MIC 3 配置.供电端口') {
  const group = src.name.substring(0, 8)
  const dest_bias = find(group + '.MIC BIAS上拉电阻挡位');
  const dest_io = find(group + '.IO供电选择');
  // console.warn('group', group)
  dest_bias.disabled = true;
  dest_io.disabled = true;
  if (src.val == '其他（IO供电或外部供电）') {
    dest_io.disabled = false;
  } else if (src.val != 'MIC_LDO (PA0)') {
    dest_bias.disabled = false;
  }

}

//----------------------------LOCALTWS编码配置逻辑控制------------------------------------------
if(src.name == '发送参数配置.编码类型'){
  const dest = find('发送参数配置.编码帧数') 
  dest.min = (src.val === 'SBC') ? 4 : 1
  dest.val =  dest.val < dest.min ? dest.min : dest.val
}


//----------------------------音量配置逻辑控制------------------------------------------

if (src.name == '音量配置.最大音量等级') {
  const dest = find('音量配置.当前音量值')
  let p = find("音量配置");
  if (src.val != dest.max) {
      if (dest.val > src.val) {
          dest.val = src.val;
          p.children[6].val = dest.val;
      }
      dest.max = src.val;
      p.children[6].max = dest.max;
  }
}
if(src.name == '音量配置.当前音量值'){
    const dest = find("音量配置");
    const child5 = dest.children[5];
    const child6 = dest.children[6];
    child6.val = child5.val;

}
if(src.name == '音量配置.'){
    const dest = find("音量配置");
    const child5 = dest.children[5];
    const child6 = dest.children[6];
    child5.val = child6.val;
}




if(src.name == '音量配置.音量表类型'){
	const dest = find('音量表')
	const max_lvl = find('音量配置.最大音量等级')
	const max_vol = find('音量配置.最大音量(dB)')
	const min_vol = find('音量配置.最小音量(dB)')
	// 子控件数组
	let temp = min_vol.val
	if(src.val == '自定义'){
		dest.insert(0, max_lvl.val)

		dest.children[0].val = min_vol.val
		//dest.children[0].src.__config__.label = "音量等级1"
		for(let i = 1; i < (dest.size - 1); i++){
			dest.children[i].val = temp + (max_vol.val - min_vol.val) / max_lvl.val
			temp = dest.children[i].val
			//dest.children[i].src.__config__.label = "音量等级" + (i+1)
		}
		dest.children[dest.size - 1].val = max_vol.val
		//dest.children[dest.size - 1].src.__config__.label = "音量等级" + (dest.size - 1 +1)
	} else {
    if(dest.size > 0){
		  dest.remove(0, dest.size)
    }
	}
}

if(src.name == '音量配置.最大音量等级' ||src.name == '音量配置.最大音量(dB)'||src.name == '音量配置.最小音量(dB)' ){
  const vtype = find ('音量配置.音量表类型')
	const dest = find('音量表')
	const max_lvl = find('音量配置.最大音量等级')
	const max_vol = find('音量配置.最大音量(dB)')
	const min_vol = find('音量配置.最小音量(dB)')
	// 子控件数组
	let temp = min_vol.val
	if(vtype.val == '自定义'){
    if(dest.size > 0){
      dest.remove(0, dest.size)
    }
		dest.insert(0, max_lvl.val)
		dest.children[0].val = min_vol.val
		// dest.children[0].src.__config__.label = "音量等级1"
		for(let i = 1; i < (dest.size - 1); i++){
			dest.children[i].val = temp + (max_vol.val - min_vol.val) / max_lvl.val
			temp = dest.children[i].val
			// dest.children[i].src.__config__.label = "音量等级" + (i + 1)
		}
		dest.children[dest.size - 1].val = max_vol.val
		// dest.children[dest.size - 1].src.__config__.label = "音量等级" + (dest.size - 1 + 1)
	} else {
    //console.log('destSize',dest.size,dest)
    if(dest.size > 0){
		  dest.remove(0, dest.size)
    }
	}
}

//----------------------------UI配置逻辑控制------------------------------------------
if(src.name == 'UI配置.UI类型'){
	let dest = find('UI配置.LED数码管屏配置.LED7脚数码管屏')
	if(src.val == 'LED数码管屏'){
		dest.val = 1
		dest.disabled = 0 
		dest = find('UI配置.LED数码管屏配置.LED屏驱动跑RAM')
		dest.val = 1
		dest.disabled = 0
		dest = find('UI配置.LCD屏配置.歌词显示')
		dest.val = 0
		dest.disabled = 1 
		dest = find('UI配置.LCD屏配置.保存歌词时间标签到flash')
		dest.val = 0
		dest.disabled = 1 
		dest = find('UI配置.LCD屏配置.驱动选择.OLED屏使能')
		dest.val = 0
		dest.disabled = 1 
		dest = find('UI配置.LCD屏配置.驱动选择.SSD1306')
		dest.val = 0
		dest.disabled = 1 
		dest = find('UI配置.LCD屏配置.驱动选择.LCD彩屏使能')
		dest.val = 0
		dest.disabled = 1
		dest = find('UI配置.LCD屏配置.驱动选择.ST7789V')
		dest.val = 0
		dest.disabled = 1
	}else if(src.val == 'OLED点阵屏（soundbar）'){
		dest.val = 0
		dest.disabled = 1 
		dest = find('UI配置.LED数码管屏配置.LED屏驱动跑RAM')
		dest.val = 0
		dest.disabled = 1 
		dest = find('UI配置.LCD屏配置.歌词显示')
		dest.val = 0
		dest.disabled = 1 
		dest = find('UI配置.LCD屏配置.保存歌词时间标签到flash')
		dest.val = 0
		dest.disabled = 1 
		dest = find('UI配置.LCD屏配置.驱动选择.OLED屏使能')
		dest.val = 1
		dest.disabled = 0
		dest = find('UI配置.LCD屏配置.驱动选择.SSD1306')
		dest.val = 1
		dest.disabled = 0
		dest = find('UI配置.LCD屏配置.驱动选择.LCD彩屏使能')
		dest.val = 0
		dest.disabled = 1
		dest = find('UI配置.LCD屏配置.驱动选择.ST7789V')
		dest.val = 0
		dest.disabled = 1
	}else if(src.val == 'LCD彩屏'){
		dest.val = 0
		dest.disabled = 1 
		dest = find('UI配置.LED数码管屏配置.LED屏驱动跑RAM')
		dest.val = 0
		dest.disabled = 1 
		dest = find('UI配置.LCD屏配置.歌词显示')
		dest.val = 0
		dest.disabled = 1 
		dest = find('UI配置.LCD屏配置.保存歌词时间标签到flash')
		dest.val = 0
		dest.disabled = 1 
		dest = find('UI配置.LCD屏配置.驱动选择.OLED屏使能')
		dest.val = 0
		dest.disabled = 1
		dest = find('UI配置.LCD屏配置.驱动选择.SSD1306')
		dest.val = 0
		dest.disabled = 1
		dest = find('UI配置.LCD屏配置.驱动选择.LCD彩屏使能')
		dest.val = 1
		dest.disabled = 0
		dest = find('UI配置.LCD屏配置.驱动选择.ST7789V')
		dest.val = 1
		dest.disabled = 0
	}
}



//----------------------IO VDD档位控制---------------------------
if(src.name == '时钟&电源.IOVDD'){
  /*let dest = find('时钟&电源.动态切换IOVDD')
  if(src.val == '3.3V' || src.val == '3.4V' || src.val == '3.5V' || src.val == '3.6V') {
    dest.val = 0;
    dest.disabled = 1;
  } else {
    dest.disabled = 0;
  }*/
  let dest = find('电池电量检测.关机电压(mV)')
  var _iovdd_val = src.val;
  _iovdd_val = parseFloat(_iovdd_val.slice(0,_iovdd_val.length-1)) * 1000;
  var _val = _iovdd_val - dest.val;
  if(_val > -300 || _val > 0){
    show('关机电压需要大于IOVDD电压0.3V，否则低压时可能引起DAC杂音');
  }
  
  dest = find('$音频配置.DAC配置.输出方式')
  show('当前DAC输出档位为' + dest.val)

}

if(src.name == '电池电量检测.关机电压(mV)'){
  const dest = find('时钟&电源.IOVDD')
  var _iovdd_val = dest.val;
  _iovdd_val = _iovdd_val.slice(0,_iovdd_val.length-1);
  var _val = src.val - parseFloat(_iovdd_val)*1000;

  if(_val < 300){
    show('关机电压需要大于IOVDD电压0.3V');
  }
}

if(src.name == '电池电量检测') {
  const dest = find('$蓝牙配置.经典蓝牙.电量显示')
    dest.val = src.groupEnable;
}

if(src.name == '经典蓝牙.电量显示' && src.val) {
  const dest = find('$电源配置.电池电量检测')
    dest.groupEnable = src.val;
}
//----------------------IO VDD档位控制---------------------------
if(src.name == 'FM配置.内置FM_AGC使能'){
	  let dest = find('FM配置.内置FM_AGC初值')
	  if(src.val)
		  dest.disabled = 1;
	  else
		  dest.disabled = 0;
}

if(src.name == '蓝牙模式选择.NORMAL模式下使能DUT测试'){
  const dest0 = find('第三方协议配置')
  const dest1 = find('BLE')
  if(dest0.groupEnable && src.val){
    show('使能DUT配置需要关闭第三方协议配置')
    dest0.groupEnable = 0;
  }
  if(dest1.groupEnable && dest0.groupEnable == 0 && src.val == 0){
    dest0.groupEnable = 1;
  }
}
//----------------------SPDIF配置控制---------------------------

if(src.name == 'HDMI ARC配置.源选择-模拟'){
	 let dest = find('HDMI ARC配置.源选择-数字')
	 let dest1 = find('OPTIAL配置.源选择-模拟')
	 let dest2 = find('COALXIAL配置.源选择-模拟')
	 if(src.val == 0){
		 dest.disabled = 0;
	 }else{
		dest.disabled = 1;
		 if(src.val == dest1.val){
			show('IO conflict with OPTIAL');
		 }
		 if(src.val == dest2.val){
			show('IO conflict with COALXIAL');
		 }	 
	 }
}

if(src.name == 'HDMI ARC配置.插入检测'){
	  let dest = find('HDMI ARC配置.HDMI-DET脚')
	  if(src.val == "无检测"){
		 // dest.src.__config__.defaultValue= undefined
		  dest.disabled = 1;
	  }else{
		  dest.disabled = 0;
	  }  
}

if(src.name == 'OPTIAL配置.源选择-模拟'){
	 let dest = find('OPTIAL配置.源选择-数字')
	 let dest1 = find('HDMI ARC配置.源选择-模拟')
	 let dest2 = find('COALXIAL配置.源选择-模拟')
	 if(src.val == 0){
		 dest.disabled = 0;
	 }else{
		dest.disabled = 1;
		 if(src.val == dest1.val){
			show('IO conflict with HDMI ARC');
		 }
		 if(src.val == dest2.val){
			show('IO conflict with COALXIAL');
		 }	 
	 }
}

if(src.name == 'COALXIAL配置.源选择-模拟'){
	let dest = find('COALXIAL配置.源选择-数字')
	let dest1 = find('HDMI ARC配置.源选择-模拟')
	let dest2 = find('OPTIAL配置.源选择-模拟')
	 if(src.val == 0){
		 dest.disabled = 0;
	 }else{
		dest.disabled = 1;
		if(src.val == dest1.val){
			show('IO conflict with HDMI ARC');
		}
		if(src.val == dest2.val){
			show('IO conflict with OPTIAL');
		}	
	 }
}

//显示隐藏脚本 status: 0 :显示 1：仅隐藏（不影响导出） 2:隐藏且不导出
if (src.name == '降噪类型配置.降噪类型') {
  let dest = find('降噪参数配置.触发时机')
 // let dest = find('降噪参数配置')
  if (src.val == '通话下行降噪') {
    if (dest) {
      dest.status = 0;
    }
  }
  if (src.val == '通用降噪') {
    if (dest) {
      dest.status = 1;
    }
  }
}


if (src.name == '第3段.High Threshold'||src.name == '第3段.Low Threshold'||src.name == '第3段.Gain') { 
    const low = find('第3段.Low Threshold', true);  
    const high = find('第3段.High Threshold', true);  
    const gain = find('第3段.Gain');  
    if (high && low){
        low.max = high.val - 0.1;
        if (low.val >= low.max){
            low.val = low.max;
        }
    }

    if (gain && high && low){
        gain.max = high.val - low.val;  
        if(gain.val > gain.max){
            gain.val = gain.max;
        }
    }
} 
if (src.name == '第2段.High Threshold'||src.name == '第2段.Low Threshold'||src.name == '第2段.Gain') { 
    const low = find('第2段.Low Threshold', true);  
    const high = find('第2段.High Threshold', true);  
    const gain = find('第2段.Gain');  
    if (high && low){
        low.max = high.val - 0.1;
        if (low.val >= low.max){
            low.val = low.max;
        }
    }
    if (gain && high && low){
        gain.max = high.val - low.val;  
        if(gain.val > gain.max){
            gain.val = gain.max;
        }
    }
} 
if (src.name == '第1段.High Threshold'||src.name == '第1段.Low Threshold'||src.name == '第1段.Gain') { 
    const low = find('第1段.Low Threshold', true);  
    const high = find('第1段.High Threshold', true);  
    const gain = find('第1段.Gain');  
    if (high && low){
        low.max = high.val - 0.1;
        if (low.val >= low.max){
            low.val = low.max;
        }
    }
    if (gain && high && low){
        gain.max = high.val - low.val;  
        if(gain.val > gain.max){
            gain.val = gain.max;
        }
    } 
} 
if (src.name == '第0段.High Threshold'||src.name == '第0段.Low Threshold'||src.name == '第0段.Gain') {
        const low = find('第0段.Low Threshold', true);  
        const high = find('第0段.High Threshold', true);  
        const gain = find('第0段.Gain');  
        if (high && low){
            low.max = high.val - 0.1;
            if (low.val >= low.max){
                low.val = low.max;
            }
        }
        if (gain && high && low){
            gain.max = high.val - low.val;  
            if(gain.val > gain.max){
                gain.val = gain.max;
            }
        }
}
//显示隐藏脚本 status: 0 :显示 1：仅隐藏（不影响导出） 2:隐藏且不导出
if ((src.name == 'Dynamic EQ Pro.nSection') || (src.name == 'Dynamic EQ Pro Ext Detector.nSection') || (src.name == 'Frequency Compressor.nSection')) {
  let dest0 = find('第0段');
  let dest1 = find('第1段');
  let dest2 = find('第2段');
  let dest3 = find('第3段');
  if (src.val == 1) {
    if (dest0) {

      dest0.status = 0;
    }
    if (dest1) {
      dest1.status = 1;
    }
    if (dest2) {
      dest2.status = 1;
    }
    if (dest3) {
      dest3.status = 1;
    }
  }
  if (src.val == 2) {
    if (dest0) {
      dest0.status = 0;
    }
    if (dest1) {
      dest1.status = 0;
    }
    if (dest2) {
      dest2.status = 1;
    }
    if (dest3) {
      dest3.status = 1;
    }
  }
  if (src.val == 3) {
    if (dest0) {
      dest0.status = 0;
    }
    if (dest1) {
      dest1.status = 0;
    }
    if (dest2) {
      dest2.status = 0;
    }
    if (dest3) {
      dest3.status = 1;
    }
  }
  if (src.val == 4) {
    if (dest0) {
      dest0.status = 0;
    }
    if (dest1) {
      dest1.status = 0;
    }
    if (dest2) {
      dest2.status = 0;
    }
    if (dest3) {
      dest3.status = 0;
    }
  }
}

if(src.name =='IO按键配置.长按复位'&&src.val){
	let p = find("IO按键配置")
  if(p.groupEnable == 0) {
    show("请先打开IO按键配置")
    src.val = 0;
    return;
  }
  let active_cnt = 0;
	for(let i = 0; i<p.size ; i++){
		const g = p.children[i]
		const dest = g.children[3]
		//dest.val = (dest.src === src.src)?1:0
    if(dest.val){
      active_cnt ++
    }
	}
  if(active_cnt > 1){
    show('长按复位只能配置一个', true)
    src.val = 0;
  }
	p = find("AD按键配置")
  if(p.groupEnable == 0){
    const k = p.children[4]
    if(k.val){
      show('长按复位只能配置一个，请关闭ADKEY长按复位配置', true)
      src.val = 0;
      return;
    }
  }

  p = find("非按键长按复位配置")
  if(p.groupEnable){
    show('长按复位只能配置一个， 请关闭板级配置的长按复位', true)
    src.val = 0;
    return;
  }
}

if(src.name =='AD按键配置.长按复位'&&src.val){
  let p = find("AD按键配置")
  if(p.groupEnable == 0){
    show("请打开AD按键配置")
    src.val = 0
    return
  }
	p = find("IO按键配置")
  if(p.groupEnable){
    for(let i = 0; i<p.size ; i++){
      const g = p.children[i]
      let dest = g.children[3]
      //dest.val = (dest.src === src.src)?1:0
      if(dest.val){
        show('长按复位只能配置一个， 请关闭IOKEY长按复位配置', true)
        src.val = 0;
        return;
      }
    }
	}
  p = find("非按键长按复位配置")
  if(p.groupEnable){
    show('长按复位只能配置一个，请关闭板级配置的长按复位', true)
    src.val = 0;
    return;
  }
}

if(src.name == 'IO按键配置') {
  if(src.groupEnable == 0){
    let p = find("IO按键配置")
    for(let i = 0; i<p.size ; i++){
      const g = p.children[i]
      const dest = g.children[3]
      //dest.val = (dest.src === src.src)?1:0
      if(dest.val){
        dest.val = 0;
      }
    }
  }
}

if(src.name == 'AD按键配置'){
  if(src.groupEnable == 0){
    const k = src.children[4]
    k.val = 0;
  }
}
if(src.name =='非按键长按复位配置'&&src.groupEnable){
  show('非按键需要进行长按复位的可以打开此配置，如样机预留复位孔', true)

  let p = find("IO按键配置")
  for(let i = 0; i<p.size ; i++){
		const g = p.children[i];
    let j = g.children[1];
    if((j.val == src.children[0].val) && p.groupEnable) {
      show('非按键的长按复位IO选择请不要与IO、AD按键配置的IO冲突', true) 
    }
		let dest = g.children[3]
		//dest.val = (dest.src === src.src)?1:0
    if(dest.val){
      show('长按复位只能配置一个, 请检查按键配置是否打开了长按复位', true)
      src.groupEnable = 0;
      return;
    }
  }
  p = find("AD按键配置")
  const k = p.children[4]
  let j = p.children[0]
  if(j.val == src.children[0].val && p.groupEnable){
    show('非按键的长按复位IO选择请不要与IO、AD按键配置的IO冲突', true) 
  }
  if(k.val){
    show('长按复位只能配置一个, 请检查按键配置是否打开了长按复位', true)
    src.groupEnable = 0;
    return;
  }
}

if(src.name == '非按键长按复位配置.复位IO' && src.groupEnable) {
  let p = find("IO按键配置")
  if(p.groupEnable){
    for(let i = 0; i<p.size ; i++){
      const g = p.children[i]
      const dest = g.children[3]
      //dest.val = (dest.src === src.src)?1:0
      if(g.children[1].val == src.val){
        show('非按键的长按复位IO选择请不要与IO、AD按键配置的IO冲突', true)
      }
    }
  }
  p = find("AD按键配置")
  if(p.groupEnable){
      let k = p.children[0]
      if(k.val == src.val){
        show('非按键的长按复位IO选择请不要与IO、AD按键配置的IO冲突', true)
      }
  }
}

if(src.name == 'TWS.MAC地址'){
  if(src.val == "使用公共地址"){
    let p = find("TWS.单台连手机也能进行配对")
    p.val = 0;
    p.disabled = 1;
    p = find("TWS.自动主从切换")
    p.val = 0;
    p.disabled = 0;
    p = find("TWS.主从电量平衡")
    p.val = 0;
    p.disabled = 0;
  }else{
    let q = find("TWS.单台连手机也能进行配对")
    q.val = 1;
    q.disabled = 0;
    q = find("TWS.自动主从切换")
    q.val = 0;
    q.disabled = 1;
    q = find("TWS.主从电量平衡")
    q.val = 0;
    q.disabled = 1;
    q = find("TWS.配对方式")
    q.val = '按键配对'
    q = find("TWS.同步关机")
    q.val = 1
  }
}

if(src.name == 'TWS.配对方式'){
  let p = find("TWS.MAC地址")
  if(p.val == "不使用公共地址" && src.val == "自动配对"){
    src.val = "按键配对"
    show("不使用公共地址只能选择按键配对", true)
  }
}

if(src.name == 'APP模式配置.FM模式'){
  const p = find("$板级配置.外置FM配置")
  if(src.val == 1 && p.groupEnable != 1){
    show('需要支持外置FM才能使用FM模式，请在板级配置中配置外置FM', true)
    src.val = 0
  }
}

if(src.name == '外置FM配置'){
  if(src.groupEnable == 0){
    const p = find('$功能配置.APP模式配置.FM模式')
    p.val = 0
  }
}

if(src.name == 'LINEIN配置.LINEIN检测配置.检测IO上拉使能' && src.val){
  const dest = find('LINEIN配置.LINEIN检测配置.检测IO下拉使能')
  if(dest.val){
    dest.val = 0;
  }
}

if(src.name == 'LINEIN配置.LINEIN检测配置.检测IO下拉使能' && src.val){
  const dest = find('LINEIN配置.LINEIN检测配置.检测IO上拉使能')
  if(dest.val){
    dest.val = 0;
  }
}

if (src.name == '时钟&电源.低功耗模式') {
  const dest1 = find('$按键配置.IR按键配置')
  if (src.val && dest1.groupEnable) {
    src.val = 0;
    show('打开IR按键无法使能低功耗');
  }
  const dest2 = find('$UI配置.UI配置')
  if (src.val && dest2.groupEnable) {
    src.val = 0;
    show('打开UI无法使能低功耗');
  }
}

if (src.name == 'IR按键配置') {
  const dest = find('$电源配置.时钟&电源.低功耗模式')
  if(src.groupEnable && dest.val){
    src.groupEnable = 0;
    show('请先关闭低功耗使能');
  }
}

if (src.name == 'UI配置') {
  const dest1 = find('$电源配置.时钟&电源.低功耗模式')
  const dest2 = find('UI配置.UI类型')
  if(src.groupEnable && dest1.val) {
    src.groupEnable = 0;
    show('使能UI需要先关闭低功耗使能');
  }
}
//----------------------离线语音识别配置控制---------------------------
if (src.name == '离线语音识别.离线语音识别'){
	const dest = find('通用音频配置.全局采样率')
	if (src.val == 1){
		show('全局采样率需要配置为48000Hz');
		dest.val = '48000Hz';
		dest.disabled = 1;
	}else{
		dest.val = '44100Hz';
		dest.disabled = 0;
	}
}

if(src.name == '升级选择.ble蓝牙升级' && src.val){
  let p = find('$蓝牙配置.BLE')
  if(p.groupEnable == 0){
    show('打开Ble蓝牙升级自动打开BLE使能')
    p.groupEnable = 1;
    p = find('$蓝牙配置.第三方协议配置')
    p.groupEnable = 1;
  }
}
if (src.name == '高级配置.Type') {
  const dest = find('StereoMtapsEcho.Repeat Time')
  if (dest) {
    if (src.val == 'FIR') {
      dest.disabled = false;
    } else {
      dest.disabled = true;
    }
  }
}

if (src.name === '流程图.位宽切换') {
  const moudle = src.moudle // 流程模块名称
  const bitWidth =  src.bitWidth
  if(moudle=='pipeline-Media') {
    const dest = find('$USB Audio')
    if(!dest) {
      show('找不到USB Audio')
      return
    }
    dest.bitWidth = bitWidth
    show('USB Audio位宽切换为' + bitWidth + 'bit')
  } else if(moudle=='pipeline-PCAudio') {
    const dest = find('$媒体')
    if(!dest) {
      show('找不到媒体')
      return
    }
    dest.bitWidth = bitWidth
    show('媒体位宽切换为' + bitWidth + 'bit')
  }
}


if (src.name == 'CrossOver.多带数') {
  let dest0 = find('Low-Band')
  let dest1 = find('Mid-Band')
  let dest2 = find('High-Band')
  let dest3 = find('Full-Band')

  let dest_mid_freq = find('CrossOver.高分频点')
  let dest_low_freq = find('CrossOver.低分频点');

  if (src.val == 2) {
    if (dest0) {
      dest0.status = 0;
    }
    if (dest1) {
      dest1.status = 1;
    }
    if (dest2) {
      dest2.status = 0;
    }
    if (dest3) {
      dest3.status = 0;
    }
    if (dest_mid_freq){
        dest_mid_freq.status = 1;
    }
    if (dest_low_freq){
        dest_low_freq.max = 20000;
    }
  }
  if (src.val == 3) {
    if (dest0) {
      dest0.status = 0;
    }
    if (dest1) {
      dest1.status = 0;
    }
    if (dest2) {
      dest2.status = 0;
    }
    if (dest3) {
      dest3.status = 0;
    }

    if (dest_mid_freq){
        dest_mid_freq.status = 0;
        if (dest_low_freq){
            dest_low_freq.max = dest_mid_freq.val;
            if (dest_low_freq.val >= dest_mid_freq.val){
                dest_low_freq.val = dest_mid_freq.val ;
            }
        }
    }

  }
}

if (src.name == 'CrossOver.低分频点') {
    const dest = find('CrossOver.高分频点');
    const band = find('CrossOver.多带数');
    if (band.val  == 3){
        if (dest) {
            dest.min = src.val;
            if (src.val > dest.val) {
                src.val = dest.val;
                // show('高分频点应当比低分频点大');
            }
        }
    }
}
if (src.name == 'CrossOver.高分频点') {
    const dest = find('CrossOver.低分频点');
    const band = find('CrossOver.多带数');
    if (band.val  == 3){
        if (dest) {
            dest.max = src.val;
            if (src.val < dest.val) {
                src.val = dest.val;
                // show('高分频点应当比低分频点大');
            }
        }
    }
}

//----------------------编码器节点配置控制---------------------------
if(src.name == '编码器参数配置.编码格式'){
	let dest0 = find('编码器参数配置.位宽')
	let dest1 = find('编码器参数配置.码率')
  let dest2 = find('编码器参数配置.声道')
	let dest3 = find('编码器参数配置.采样率')
  dest1.disabled = false;
  dest2.disabled = false;
  dest3.disabled = false;
	
	if (src.val == 'PCM') {
		if (dest0) {
			dest0.disabled = false;
			dest0.options[1].label = "定点24位";
			dest0.options[2].label = "定点32位";
			dest0.options[3].label = "浮点32位";
		}
		if (dest1) {
			dest1.status = 1;
		}	
	} else if(src.val == 'AMR'){
        if(dest0){
          dest0.val = "定点16位";
			    dest0.disabled = true;
        }
        if(dest2){
          dest2.val = "单声道";
          dest2.disabled = true;
        }
        if(dest3){
          dest3.val = "8000";
          dest3.disabled = true;
        }
  }else {

		if (dest1) {
			dest1.disabled = false;
			if(src.val == 'JLA'){
			dest1.disabled = true;
			dest1.val = "128000";
		}
			dest1.status = 0;
		}			
		
	}
}


//reverb
if ((src.name == 'Reverb.Buffer Param.EarlyReflection Bufsize Factor') 
        || (src.name == 'Reverb.LateReflection Param.LateReflection Pre Delay') ) {
    const srct = find('Reverb.Buffer Param.EarlyReflection Bufsize Factor')
    const dest = find('Reverb.LateReflection Param.LateReflection Pre Delay')
    if (dest && srct) { 
        dest.max = srct.val
        if (dest.val > srct.val) {
             dest.val= srct.val
        }
    } 
}


//----------------------LINEIN模式配置提示---------------------------
if (src.name == 'APP模式配置.LINEIN模式') {
  const dest = find('APP模式配置.LINEIN模式')  // linein模式功能打开后要使能对应的LINEIN检测配置,否则切换模式的时候可能出现死机
  if (dest.val) {
    show('LINEIN模式功能已打开, 需要在板级配置中使能对应的LINEIN检测配置和LINEIN配置!',true)
  } 
}

if (src.name == '1T3配置.1T3使能' && src.val) {
  let p = find("$CIS配置.CIS功能配置.主机使能")
  if (!p.val) {
    p.val = 1;
  }

  p = find("$BIS配置.BIS功能配置.发送使能")
  if (p.val) {
    p.val = 0;
  }

  p = find("$BIS配置.BIS功能配置.接收使能")
  if (p.val) {
    p.val = 0;
  }

  p =  find('公共配置.编解码声道数')
  if (p.val != '单声道') {
    p.val = '单声道';
  }
  
  p = find('公共配置.接收端解码输出')
  if (p.val != '左声道') {
	p.val = '左声道';
  }
  
  p = find("$CIS配置.CIS功能配置.按键同步")
  if (p.val) {
    p.val = 0;
  }

  p = find("$CIS配置.CIS功能配置.连接角色")
  if (p.val != '主机') {
    p.val = '主机';
  }

  p = find("$CIS配置.CIS功能配置.连接方式")
  if (p.val != '两发一收') {
    p.val = '两发一收';
  }   

  p = find("$CIS配置.CIS功能配置.音频传输方式")
  if (p.val != '单向') {
    p.val = '单向';
  }  
  
  p = find('$蓝牙配置.经典蓝牙.sbcBitPool')
  let d = find('$蓝牙配置.经典蓝牙.AAC码率')
  if (p.val != 38 || d.val != 131072) {
    p.val = 38;
    d.val = 131072;
    show('开启LE AUDIO后sbcBitPool改为38，AAC最大码率改为131Kbps', true)
  }
  
  const dest = find("$公共配置.公共配置.编解码格式")
  dest.options[1].disabled = false;

  show('开启1T3功能后，配置默认发生以下变化：<br/>(1)自动关闭big；<br/>(2)打开cig主机使能并设为两发一收单向工作模式；<br/>(3)接收端解码输出声道改为左声道。', true)
}

if(src.name == '公共配置.编解码格式' && src.val == 'JLA_LW') {
	let a = find("$CIS配置.CIS参数配置.1T3模式.码率(bps)")
	let b = find("$CIS配置.CIS参数配置.1T3模式.从到主重发次数")
	a.val = 96000;
	b.val = 4;
	show('1T3模式 已经设置码率为96000，从到主重发次数设置为4', true)
}


if (src.name == '1T3配置.1T3使能' && !src.val) {
  show('关闭1T3功能后，请手动还原被修改的配置', true)

  let a = find("$BIS配置.BIS功能配置.发送使能")
  let b = find("$BIS配置.BIS功能配置.接收使能")
  let p = find("$CIS配置.CIS功能配置.主机使能")
  let d = find("$CIS配置.CIS功能配置.从机使能")
  if (!a.val && !b.val && !p.val && !d.val) {
    p = find('$蓝牙配置.经典蓝牙.sbcBitPool')
    d = find('$蓝牙配置.经典蓝牙.AAC码率')
    if (p.val != 53 || d.val != 320000) {
      p.val = 53;
      d.val = 320000;
    }
  }
}

if(src.name == '公共配置.编解码格式' && src.val == 'JLA') {
  let p = find("$音频配置.解码相关配置.自定义解码格式.JLA")
  let d = find("$音频配置.编码配置.JLA")
  p.val = 1;
  d.val = 1;

  p = find("$公共配置.1T3配置.1T3使能")
  if (p.val) {
    let a = find("$CIS配置.CIS参数配置.1T3模式.码率(bps)")
    let b = find("$CIS配置.CIS参数配置.1T3模式.从到主重发次数")
    a.val = 64000;
    b.val = 5;
  }
}

if (src.name == '第三方协议配置' && src.groupEnable) {
  show('需注意：<br/> \
    1.第三方协议配置使能情况下，不能关闭BLE使能<br/> \
    2.DUT使能情况下不允许打开第三方协议配置', true)

  let p = find('BLE')
  let dest1 = find('蓝牙模式选择.NORMAL模式下使能DUT测试')

  if(dest1.val) {
    dest1.val = 0;
  }

  if (src.groupEnable && !p.groupEnable) {
    p.groupEnable = 1;
  }
}

if (src.name == '第三方协议配置' && !src.groupEnable) {
	const dest0 = find('BLE')
  const dest1 = find('蓝牙模式选择.NORMAL模式下使能DUT测试')
  if(dest0.groupEnable && dest1.val == 0) {
    show('非DUT模式关闭第三方协议配置，BLE使能也同步关闭')
    dest0.groupEnable = 0;
  }
}

if (src.name == 'BLE' && !src.groupEnable) {
  let p = find('$蓝牙配置.第三方协议配置')
  if (p.groupEnable) {
    p.groupEnable = 0;
  }
}

if (src.name == 'BIS功能配置.发送使能' && src.val) {
  show('需注意：<br/> \
    1.BIS和CIS不支持同时打开，需要先关闭CIS主机和从机使能<br/> \
    2.自动关闭第三方协议配置<br/> \
    3.sbcBitPool自动改为38，AAC最大码率自动改为131Kbps', true)

  let p = find("$CIS配置.CIS功能配置.主机使能")
  let d = find("$CIS配置.CIS功能配置.从机使能")
  if (p.val || d.val) {
	  p.val = 0;
    d.val = 0;
  }

  p = find("$蓝牙配置.BLE")
  if (!p.groupEnable) {
    p.groupEnable = 1;
  }

  p = find('$蓝牙配置.第三方协议配置')
  if(p.groupEnable){
    p.groupEnable = 0;
  }
  
  if (src.val) {
    p = find('$蓝牙配置.经典蓝牙.sbcBitPool')
    d = find('$蓝牙配置.经典蓝牙.AAC码率')
    if (p.val != 38 || d.val != 131072) {
      p.val = 38;
      d.val = 131072;
    }
  }
}

if (src.name == 'BIS功能配置.接收使能' && src.val) {
  show('需注意：<br/> \
    1.BIS和CIS不支持同时打开，需要先关闭CIS主机和从机使能<br/> \
    2.自动关闭第三方协议配置<br/> \
    3.sbcBitPool自动改为38，AAC最大码率自动改为131Kbps', true)

  let p = find("$蓝牙配置.BLE")
  if (!p.groupEnable) {
    p.groupEnable = 1;
  }

  p = find('$蓝牙配置.第三方协议配置')
  if(p.groupEnable){
    p.groupEnable = 0;
  }

  p = find("$CIS配置.CIS功能配置.主机使能")
  let d = find("$CIS配置.CIS功能配置.从机使能")
  if (p.val || d.val) {
	  p.val = 0;
    d.val = 0;
  }

  if (src.val) {
    p = find('$蓝牙配置.经典蓝牙.sbcBitPool')
    d = find('$蓝牙配置.经典蓝牙.AAC码率')
    if (p.val != 38 || d.val != 131072) {
      p.val = 38;
      d.val = 131072;
    }
  }
}

if (src.name == 'BIS功能配置.发送使能' && !src.val) {
  show('关闭BIS后sbcBitPool改为53，AAC最大码率改为320Kbps')

  let p = find('$蓝牙配置.经典蓝牙.sbcBitPool')
  let d = find('$蓝牙配置.经典蓝牙.AAC码率')
  let t = find('BIS功能配置.接收使能')
  if ((p.val != 53 || d.val != 320000) && !t.val) {
    p.val = 53;
    d.val = 320000;
  }
}

if (src.name == 'BIS功能配置.接收使能' && !src.val) {
  show('关闭BIS后sbcBitPool改为53，AAC最大码率改为320Kbps')

  let p = find('$蓝牙配置.经典蓝牙.sbcBitPool')
  let d = find('$蓝牙配置.经典蓝牙.AAC码率')
  let t = find('BIS功能配置.接收使能')
  if ((p.val != 53 || d.val != 320000) && !t.val) {
    p.val = 53;
    d.val = 320000;
  }
}

if (src.name == 'BIS功能配置.音量同步' && src.val) {
  let p = find("BIS功能配置.自定义数据同步")
  if (!p.val) {
	  p.val = 1;
	  show('音量同步需要先打开自定义数据同步功能', true)
  }
}

if (src.name == 'BIS功能配置.自定义数据同步' && !src.val) {
  let p = find("BIS功能配置.音量同步")
  if (p.val) {
	  p.val = 0;
	  show('音量同步需要先打开自定义数据同步功能', true)
  }
}

if (src.name == 'CIS功能配置.主机使能' && src.val) {
  show('需注意：<br/> \
    1.BIS和CIS不支持同时打开，需要先关闭BIS发送和接收使能<br/> \
    2.自动关闭第三方协议配置<br/> \
    3.sbcBitPool自动改为38，AAC最大码率自动改为131Kbps', true)

  let p = find("$BIS配置.BIS功能配置.发送使能")
  let d = find("$BIS配置.BIS功能配置.接收使能")
  if (p.val || d.val) {
	  p.val = 0;
    d.val = 0;
  }

  p = find("$蓝牙配置.BLE")
  if (!p.groupEnable) {
	  p.groupEnable = 1;
  }

  p = find('$蓝牙配置.第三方协议配置')
  if(p.groupEnable){
    p.groupEnable = 0;
  }

  if (src.val) {
    p = find('$蓝牙配置.经典蓝牙.sbcBitPool')
    d = find('$蓝牙配置.经典蓝牙.AAC码率')
    if (p.val != 38 || d.val != 131072) {
      p.val = 38;
      d.val = 131072;
    }
  }
}

if (src.name == 'CIS功能配置.主机使能' && !src.val) {
  show('关闭CIS后sbcBitPool改为53，AAC最大码率改为320Kbps', true)

  let p = find('$蓝牙配置.经典蓝牙.sbcBitPool')
  let d = find('$蓝牙配置.经典蓝牙.AAC码率')
  let t = find('CIS功能配置.从机使能')
  if ((p.val != 53 || d.val != 320000) && !t.val) {
    p.val = 53;
    d.val = 320000;
  }
}

if (src.name == 'CIS功能配置.从机使能' && src.val) {
  show('需注意：<br/> \
    1.BIS和CIS不支持同时打开，需要先关闭BIS发送和接收使能<br/> \
    2.自动关闭第三方协议配置<br/> \
    3.sbcBitPool自动改为38，AAC最大码率自动改为131Kbps', true)

  let p = find("$BIS配置.BIS功能配置.发送使能")
  let d = find("$BIS配置.BIS功能配置.接收使能")
  if (p.val || d.val) {
	  p.val = 0;
    d.val = 0;
  }

  p = find("$蓝牙配置.BLE")
  if (!p.groupEnable) {
	  p.groupEnable = 1;
  }

  p = find('$蓝牙配置.第三方协议配置')
  if(p.groupEnable){
    p.groupEnable = 0;
  }

  if (src.val) {
    p = find('$蓝牙配置.经典蓝牙.sbcBitPool')
    p = find('$蓝牙配置.经典蓝牙.AAC码率')
    if (p.val != 38 || d.val != 131072) {
      p.val = 38;
      d.val = 131072;
    }
  }
}

if (src.name == 'CIS功能配置.从机使能' && !src.val) {
  show('关闭CIS后sbcBitPool改为53，AAC最大码率改为320Kbps', true)

  let p = find('$蓝牙配置.经典蓝牙.sbcBitPool')
  let d = find('$蓝牙配置.经典蓝牙.AAC码率')
  let t = find('CIS功能配置.主机使能')
  if ((p.val != 53 || d.val != 320000) && !t.val) {
    p.val = 53;
    d.val = 320000;
  }
}

//BIS LINEIN模式延时约束
if (src.name == '公共配置.编解码声道数' || 
	  src.name == '公共配置.帧持续时间' || 
	  src.name == '公共配置.采样率' || 
	  src.name == 'BIS参数配置.LINEIN模式.发包间隔(us)' || 
	  src.name == 'BIS参数配置.LINEIN模式.蓝牙重发次数' || 
	  src.name == 'BIS参数配置.LINEIN模式.蓝牙预发包' || 
	  src.name == 'BIS参数配置.LINEIN模式.码率(bps)' || 
	  src.name == 'BIS参数配置.LINEIN模式.发送延时(us)') {
	  	
	const frame_dms = find('$公共配置.公共配置.帧持续时间')
	const latency =  find('$BIS配置.BIS参数配置.LINEIN模式.发送延时(us)')
	const sdu_period = find('$BIS配置.BIS参数配置.LINEIN模式.发包间隔(us)')
	const rtn = find('$BIS配置.BIS参数配置.LINEIN模式.蓝牙重发次数')
	const mtl = find('$BIS配置.BIS参数配置.LINEIN模式.蓝牙预发包')
	const bitrate = find('$BIS配置.BIS参数配置.LINEIN模式.码率(bps)')
	
  if(!latency){
  	show('未找到发送延时(us)', true)
    return
  }

  let frame_duration = 10000;
  if (frame_dms.val == '2.5ms') {
  	frame_duration = 2500;
  } else if (frame_dms.val == '5ms') {
  	frame_duration = 5000;
  } else if (frame_dms.val == '7.5ms') {
  	frame_duration = 7500;		
  } else if (frame_dms.val == '10ms') {
  	frame_duration = 10000;
  }
  let frame_num = 1000000 / frame_duration;
  let packet_num = 1000000 / sdu_period.val;
  let real_bitrate = bitrate.val + (frame_num * 2 * 8) + (packet_num * 11 * 8);
  let sync_delay = ((real_bitrate / 1000) * (frame_duration / 100) / 2 + 1500) * (rtn.val + 1) / 10;
  let enc_time = frame_duration / 2;
 	latency.min = 1000 + enc_time + sync_delay + sdu_period.val + mtl.val * sdu_period.val;
  if(latency.val < latency.min){
    latency.val = latency.min;
    show('LINEIN模式延时应根据发包间隔、重发、预发包和码率设置', true)
  }
	
}

//BIS PC从机模式延时约束
if (src.name == '公共配置.编解码声道数' || 
	  src.name == '公共配置.帧持续时间' || 
	  src.name == '公共配置.采样率' || 
	  src.name == 'BIS参数配置.PC从机模式.发包间隔(us)' || 
	  src.name == 'BIS参数配置.PC从机模式.蓝牙重发次数' || 
	  src.name == 'BIS参数配置.PC从机模式.蓝牙预发包' || 
	  src.name == 'BIS参数配置.PC从机模式.码率(bps)' || 
	  src.name == 'BIS参数配置.PC从机模式.发送延时(us)') {
	  	
	const frame_dms = find('$公共配置.公共配置.帧持续时间')
	const latency =  find('$BIS配置.BIS参数配置.PC从机模式.发送延时(us)')
	const sdu_period = find('$BIS配置.BIS参数配置.PC从机模式.发包间隔(us)')
	const rtn = find('$BIS配置.BIS参数配置.PC从机模式.蓝牙重发次数')
	const mtl = find('$BIS配置.BIS参数配置.PC从机模式.蓝牙预发包')
	const bitrate = find('$BIS配置.BIS参数配置.PC从机模式.码率(bps)')
	
  if(!latency){
  	show('未找到发送延时(us)', true)
    return
  }

  let frame_duration = 10000;
  if (frame_dms.val == '2.5ms') {
  	frame_duration = 2500;
  } else if (frame_dms.val == '5ms') {
  	frame_duration = 5000;
  } else if (frame_dms.val == '7.5ms') {
  	frame_duration = 7500;		
  } else if (frame_dms.val == '10ms') {
  	frame_duration = 10000;
  }
  let frame_num = 1000000 / frame_duration;
  let packet_num = 1000000 / sdu_period.val;
  let real_bitrate = bitrate.val + (frame_num * 2 * 8) + (packet_num * 11 * 8);
  let sync_delay = ((real_bitrate / 1000) * (frame_duration / 100) / 2 + 1500) * (rtn.val + 1) / 10;
  let enc_time = frame_duration / 2;
 	latency.min = 1000 + enc_time + sync_delay + sdu_period.val + mtl.val * sdu_period.val;
  if(latency.val < latency.min){
    latency.val = latency.min;
    show('PC从机模式延时应根据发包间隔、重发、预发包和码率设置', true)
  }
	
}

//BIS IIS输入模式延时约束
if (src.name == '公共配置.编解码声道数' || 
	  src.name == '公共配置.帧持续时间' || 
	  src.name == '公共配置.采样率' || 
	  src.name == 'BIS参数配置.IIS输入模式.发包间隔(us)' || 
	  src.name == 'BIS参数配置.IIS输入模式.蓝牙重发次数' || 
	  src.name == 'BIS参数配置.IIS输入模式.蓝牙预发包' || 
	  src.name == 'BIS参数配置.IIS输入模式.码率(bps)' || 
	  src.name == 'BIS参数配置.IIS输入模式.发送延时(us)') {
	  	
	const frame_dms = find('$公共配置.公共配置.帧持续时间')
	const latency =  find('$BIS配置.BIS参数配置.IIS输入模式.发送延时(us)')
	const sdu_period = find('$BIS配置.BIS参数配置.IIS输入模式.发包间隔(us)')
	const rtn = find('$BIS配置.BIS参数配置.IIS输入模式.蓝牙重发次数')
	const mtl = find('$BIS配置.BIS参数配置.IIS输入模式.蓝牙预发包')
	const bitrate = find('$BIS配置.BIS参数配置.IIS输入模式.码率(bps)')
	
  if(!latency){
  	show('未找到发送延时(us)', true)
    return
  }

  let frame_duration = 10000;
  if (frame_dms.val == '2.5ms') {
  	frame_duration = 2500;
  } else if (frame_dms.val == '5ms') {
  	frame_duration = 5000;
  } else if (frame_dms.val == '7.5ms') {
  	frame_duration = 7500;		
  } else if (frame_dms.val == '10ms') {
  	frame_duration = 10000;
  }
  let frame_num = 1000000 / frame_duration;
  let packet_num = 1000000 / sdu_period.val;
  let real_bitrate = bitrate.val + (frame_num * 2 * 8) + (packet_num * 11 * 8);
  let sync_delay = ((real_bitrate / 1000) * (frame_duration / 100) / 2 + 1500) * (rtn.val + 1) / 10;
  let enc_time = frame_duration / 2;
 	latency.min = 1000 + enc_time + sync_delay + sdu_period.val + mtl.val * sdu_period.val;
  if(latency.val < latency.min){
    latency.val = latency.min;
    show('IIS输入模式延时应根据发包间隔、重发、预发包和码率设置', true)
  }
	
}

//BIS MIC输入模式延时约束
if (src.name == '公共配置.编解码声道数' || 
	  src.name == '公共配置.帧持续时间' || 
	  src.name == '公共配置.采样率' || 
	  src.name == 'BIS参数配置.MIC输入模式.发包间隔(us)' || 
	  src.name == 'BIS参数配置.MIC输入模式.蓝牙重发次数' || 
	  src.name == 'BIS参数配置.MIC输入模式.蓝牙预发包' || 
	  src.name == 'BIS参数配置.MIC输入模式.码率(bps)' || 
	  src.name == 'BIS参数配置.MIC输入模式.发送延时(us)') {
	  	
	const frame_dms = find('$公共配置.公共配置.帧持续时间')
	const latency =  find('$BIS配置.BIS参数配置.MIC输入模式.发送延时(us)')
	const sdu_period = find('$BIS配置.BIS参数配置.MIC输入模式.发包间隔(us)')
	const rtn = find('$BIS配置.BIS参数配置.MIC输入模式.蓝牙重发次数')
	const mtl = find('$BIS配置.BIS参数配置.MIC输入模式.蓝牙预发包')
	const bitrate = find('$BIS配置.BIS参数配置.MIC输入模式.码率(bps)')
	
  if(!latency){
  	show('未找到发送延时(us)', true)
    return
  }

  let frame_duration = 10000;
  if (frame_dms.val == '2.5ms') {
  	frame_duration = 2500;
  } else if (frame_dms.val == '5ms') {
  	frame_duration = 5000;
  } else if (frame_dms.val == '7.5ms') {
  	frame_duration = 7500;		
  } else if (frame_dms.val == '10ms') {
  	frame_duration = 10000;
  }
  let frame_num = 1000000 / frame_duration;
  let packet_num = 1000000 / sdu_period.val;
  let real_bitrate = bitrate.val + (frame_num * 2 * 8) + (packet_num * 11 * 8);
  let sync_delay = ((real_bitrate / 1000) * (frame_duration / 100) / 2 + 1500) * (rtn.val + 1) / 10;
  let enc_time = frame_duration / 2;
  //TODO tx delay需要加上 --蓝牙
 	latency.min = 1000 + enc_time + sync_delay + sdu_period.val + mtl.val * sdu_period.val;
  if(latency.val < latency.min){
    latency.val = latency.min;
    show('MIC输入模式延时应根据发包间隔、重发、预发包和码率设置', true)
  }
}


//CIS LINEIN模式延时约束
if (src.name == '公共配置.编解码声道数' || 
	  src.name == '公共配置.帧持续时间' || 
	  src.name == '公共配置.采样率' || 
	  src.name == 'CIS功能配置.连接角色' || 
	  src.name == 'CIS功能配置.连接方式' || 
	  src.name == 'CIS功能配置.音频传输方式' || 
	  src.name == 'CIS参数配置.LINEIN模式.发包间隔(us)' || 
	  src.name == 'CIS参数配置.LINEIN模式.主到从重发次数' || 
	  src.name == 'CIS参数配置.LINEIN模式.从到主重发次数' || 
	  src.name == 'CIS参数配置.LINEIN模式.主到从预发包' || 
	  src.name == 'CIS参数配置.LINEIN模式.从到主预发包' || 
	  src.name == 'CIS参数配置.LINEIN模式.码率(bps)' || 
	  src.name == 'CIS参数配置.LINEIN模式.发送延时(us)') {
	  	 	
	const frame_dms = find('$公共配置.公共配置.帧持续时间')
	const latency =  find('$CIS配置.CIS参数配置.LINEIN模式.发送延时(us)')
	const sdu_period = find('$CIS配置.CIS参数配置.LINEIN模式.发包间隔(us)')
	const master_rtn = find('$CIS配置.CIS参数配置.LINEIN模式.主到从重发次数')
	const master_mtl = find('$CIS配置.CIS参数配置.LINEIN模式.主到从预发包')
	const slave_rtn = find('$CIS配置.CIS参数配置.LINEIN模式.从到主重发次数')
	const slave_mtl = find('$CIS配置.CIS参数配置.LINEIN模式.从到主预发包')
	const bitrate = find('$CIS配置.CIS参数配置.LINEIN模式.码率(bps)')
	const role = find('$CIS配置.CIS功能配置.连接角色')
	const connect_type = find('$CIS配置.CIS功能配置.连接方式')
	const transmit_type = find('$CIS配置.CIS功能配置.音频传输方式')
	
  if(!latency){
  	show('未找到发送延时(us)', true)
    return
  }
	
	let cis_channel = 2;//connect_type.val == '一发一收' ? 1 : 2;
	if (connect_type.val == '一发一收') {
		cis_channel = 1;
	}
	let duplex = 1//transmit_type.val == '双向' 2 : 1;
	if (transmit_type.val == '双向') {
		duplex = 2;
	}
  let frame_duration = 10000;
  let rtn_num = master_rtn.val;//role.val == '主机' ? master_rtn.val : slave_rtn.val;
  if (role.val == '从机') {
  	rtn_num = slave_rtn.val;
  }
  let mtl_num = master_mtl.val;//role.val == '主机' ? master_mtl.val : slave_mtl.val;
  if (role.val == '从机') {
  	mtl_num = slave_mtl.val;
  }
  if (frame_dms.val == '2.5ms') {
  	frame_duration = 2500;
  } else if (frame_dms.val == '5ms') {
  	frame_duration = 5000;
  } else if (frame_dms.val == '7.5ms') {
  	frame_duration = 7500;		
  } else if (frame_dms.val == '10ms') {
  	frame_duration = 10000;
  }
  let frame_num = 1000000 / frame_duration;
  let packet_num = 1000000 / sdu_period.val;
  let real_bitrate = bitrate.val + (frame_num * 2 * 8) + (packet_num * 11 * 8);
  let sync_delay = (((real_bitrate / 1000) * (frame_duration / 100) + (2 + 4) * 80) / 2 + 1500 + 440) * (rtn_num + 1) * duplex * cis_channel / 10;
  let enc_time = frame_duration / 2;
  //TODO tx delay需要加上 --蓝牙
 	latency.min = 1000 + enc_time + sync_delay + sdu_period.val + mtl_num * sdu_period.val;
  if(latency.val < latency.min){
    latency.val = latency.min;
    show('CIS LINEIN模式延时应根据发包间隔、重发、预发包和码率设置', true)
  }
}

//CIS PC从机模式延时约束
if (src.name == '公共配置.编解码声道数' || 
	  src.name == '公共配置.帧持续时间' || 
	  src.name == '公共配置.采样率' || 
	  src.name == 'CIS功能配置.连接角色' || 
	  src.name == 'CIS功能配置.连接方式' || 
	  src.name == 'CIS功能配置.音频传输方式' || 
	  src.name == 'CIS参数配置.PC从机模式.发包间隔(us)' || 
	  src.name == 'CIS参数配置.PC从机模式.主到从重发次数' || 
	  src.name == 'CIS参数配置.PC从机模式.从到主重发次数' || 
	  src.name == 'CIS参数配置.PC从机模式.主到从预发包' || 
	  src.name == 'CIS参数配置.PC从机模式.从到主预发包' || 
	  src.name == 'CIS参数配置.PC从机模式.码率(bps)' || 
	  src.name == 'CIS参数配置.PC从机模式.发送延时(us)') {
	  	 	
	const frame_dms = find('$公共配置.公共配置.帧持续时间')
	const latency =  find('$CIS配置.CIS参数配置.PC从机模式.发送延时(us)')
	const sdu_period = find('$CIS配置.CIS参数配置.PC从机模式.发包间隔(us)')
	const master_rtn = find('$CIS配置.CIS参数配置.PC从机模式.主到从重发次数')
	const master_mtl = find('$CIS配置.CIS参数配置.PC从机模式.主到从预发包')
	const slave_rtn = find('$CIS配置.CIS参数配置.PC从机模式.从到主重发次数')
	const slave_mtl = find('$CIS配置.CIS参数配置.PC从机模式.从到主预发包')
	const bitrate = find('$CIS配置.CIS参数配置.PC从机模式.码率(bps)')
	const role = find('$CIS配置.CIS功能配置.连接角色')
	const connect_type = find('$CIS配置.CIS功能配置.连接方式')
	const transmit_type = find('$CIS配置.CIS功能配置.音频传输方式')
	
  if(!latency){
  	show('未找到发送延时(us)', true)
    return
  }
	
	let cis_channel = 2;//connect_type.val == '一发一收' ? 1 : 2;
	if (connect_type.val == '一发一收') {
		cis_channel = 1;
	}
	let duplex = 1//transmit_type.val == '双向' 2 : 1;
	if (transmit_type.val == '双向') {
		duplex = 2;
	}
  let frame_duration = 10000;
  let rtn_num = master_rtn.val;//role.val == '主机' ? master_rtn.val : slave_rtn.val;
  if (role.val == '从机') {
  	rtn_num = slave_rtn.val;
  }
  let mtl_num = master_mtl.val;//role.val == '主机' ? master_mtl.val : slave_mtl.val;
  if (role.val == '从机') {
  	mtl_num = slave_mtl.val;
  }
  if (frame_dms.val == '2.5ms') {
  	frame_duration = 2500;
  } else if (frame_dms.val == '5ms') {
  	frame_duration = 5000;
  } else if (frame_dms.val == '7.5ms') {
  	frame_duration = 7500;		
  } else if (frame_dms.val == '10ms') {
  	frame_duration = 10000;
  }
  let frame_num = 1000000 / frame_duration;
  let packet_num = 1000000 / sdu_period.val;
  let real_bitrate = bitrate.val + (frame_num * 2 * 8) + (packet_num * 11 * 8);
  let sync_delay = (((real_bitrate / 1000) * (frame_duration / 100) + (2 + 4) * 80) / 2 + 1500 + 440) * (rtn_num + 1) * duplex * cis_channel / 10;
  let enc_time = frame_duration / 2;
  //TODO tx delay需要加上 --蓝牙
 	latency.min = 1000 + enc_time + sync_delay + sdu_period.val + mtl_num * sdu_period.val;
  if(latency.val < latency.min){
    latency.val = latency.min;
    show('CIS PC从机模式延时应根据发包间隔、重发、预发包和码率设置', true)
  }
}

//CIS IIS输入模式延时约束
if (src.name == '公共配置.编解码声道数' || 
	  src.name == '公共配置.帧持续时间' || 
	  src.name == '公共配置.采样率' || 
	  src.name == 'CIS功能配置.连接角色' || 
	  src.name == 'CIS功能配置.连接方式' || 
	  src.name == 'CIS功能配置.音频传输方式' || 
	  src.name == 'CIS参数配置.IIS输入模式.发包间隔(us)' || 
	  src.name == 'CIS参数配置.IIS输入模式.主到从重发次数' || 
	  src.name == 'CIS参数配置.IIS输入模式.从到主重发次数' || 
	  src.name == 'CIS参数配置.IIS输入模式.主到从预发包' || 
	  src.name == 'CIS参数配置.IIS输入模式.从到主预发包' || 
	  src.name == 'CIS参数配置.IIS输入模式.码率(bps)' || 
	  src.name == 'CIS参数配置.IIS输入模式.发送延时(us)') {
	  	 	
	const frame_dms = find('$公共配置.公共配置.帧持续时间')
	const latency =  find('$CIS配置.CIS参数配置.IIS输入模式.发送延时(us)')
	const sdu_period = find('$CIS配置.CIS参数配置.IIS输入模式.发包间隔(us)')
	const master_rtn = find('$CIS配置.CIS参数配置.IIS输入模式.主到从重发次数')
	const master_mtl = find('$CIS配置.CIS参数配置.IIS输入模式.主到从预发包')
	const slave_rtn = find('$CIS配置.CIS参数配置.IIS输入模式.从到主重发次数')
	const slave_mtl = find('$CIS配置.CIS参数配置.IIS输入模式.从到主预发包')
	const bitrate = find('$CIS配置.CIS参数配置.IIS输入模式.码率(bps)')
	const role = find('$CIS配置.CIS功能配置.连接角色')
	const connect_type = find('$CIS配置.CIS功能配置.连接方式')
	const transmit_type = find('$CIS配置.CIS功能配置.音频传输方式')
	
  if(!latency){
  	show('未找到发送延时(us)', true)
    return
  }
	
	let cis_channel = 2;//connect_type.val == '一发一收' ? 1 : 2;
	if (connect_type.val == '一发一收') {
		cis_channel = 1;
	}
	let duplex = 1//transmit_type.val == '双向' 2 : 1;
	if (transmit_type.val == '双向') {
		duplex = 2;
	}
  let frame_duration = 10000;
  let rtn_num = master_rtn.val;//role.val == '主机' ? master_rtn.val : slave_rtn.val;
  if (role.val == '从机') {
  	rtn_num = slave_rtn.val;
  }
  let mtl_num = master_mtl.val;//role.val == '主机' ? master_mtl.val : slave_mtl.val;
  if (role.val == '从机') {
  	mtl_num = slave_mtl.val;
  }
  if (frame_dms.val == '2.5ms') {
  	frame_duration = 2500;
  } else if (frame_dms.val == '5ms') {
  	frame_duration = 5000;
  } else if (frame_dms.val == '7.5ms') {
  	frame_duration = 7500;		
  } else if (frame_dms.val == '10ms') {
  	frame_duration = 10000;
  }
  let frame_num = 1000000 / frame_duration;
  let packet_num = 1000000 / sdu_period.val;
  let real_bitrate = bitrate.val + (frame_num * 2 * 8) + (packet_num * 11 * 8);
  let sync_delay = (((real_bitrate / 1000) * (frame_duration / 100) + (2 + 4) * 80) / 2 + 1500 + 440) * (rtn_num + 1) * duplex * cis_channel / 10;
  let enc_time = frame_duration / 2;
  //TODO tx delay需要加上 --蓝牙
 	latency.min = 1000 + enc_time + sync_delay + sdu_period.val + mtl_num * sdu_period.val;
  if(latency.val < latency.min){
    latency.val = latency.min;
    show('CIS IIS输入模式延时应根据发包间隔、重发、预发包和码率设置', true)
  }
}

//CIS MIC输入模式延时约束
if (src.name == '公共配置.编解码声道数' || 
	  src.name == '公共配置.帧持续时间' || 
	  src.name == '公共配置.采样率' || 
	  src.name == 'CIS功能配置.连接角色' || 
	  src.name == 'CIS功能配置.连接方式' || 
	  src.name == 'CIS功能配置.音频传输方式' || 
	  src.name == 'CIS参数配置.MIC输入模式.发包间隔(us)' || 
	  src.name == 'CIS参数配置.MIC输入模式.主到从重发次数' || 
	  src.name == 'CIS参数配置.MIC输入模式.从到主重发次数' || 
	  src.name == 'CIS参数配置.MIC输入模式.主到从预发包' || 
	  src.name == 'CIS参数配置.MIC输入模式.从到主预发包' || 
	  src.name == 'CIS参数配置.MIC输入模式.码率(bps)' || 
	  src.name == 'CIS参数配置.MIC输入模式.发送延时(us)') {
	  	 	
	const frame_dms = find('$公共配置.公共配置.帧持续时间')
	const latency =  find('$CIS配置.CIS参数配置.MIC输入模式.发送延时(us)')
	const sdu_period = find('$CIS配置.CIS参数配置.MIC输入模式.发包间隔(us)')
	const master_rtn = find('$CIS配置.CIS参数配置.MIC输入模式.主到从重发次数')
	const master_mtl = find('$CIS配置.CIS参数配置.MIC输入模式.主到从预发包')
	const slave_rtn = find('$CIS配置.CIS参数配置.MIC输入模式.从到主重发次数')
	const slave_mtl = find('$CIS配置.CIS参数配置.MIC输入模式.从到主预发包')
	const bitrate = find('$CIS配置.CIS参数配置.MIC输入模式.码率(bps)')
	const role = find('$CIS配置.CIS功能配置.连接角色')
	const connect_type = find('$CIS配置.CIS功能配置.连接方式')
	const transmit_type = find('$CIS配置.CIS功能配置.音频传输方式')
	
  if(!latency){
  	show('未找到发送延时(us)', true)
    return
  }
	
	let cis_channel = 2;//connect_type.val == '一发一收' ? 1 : 2;
	if (connect_type.val == '一发一收') {
		cis_channel = 1;
	}
	let duplex = 1//transmit_type.val == '双向' 2 : 1;
	if (transmit_type.val == '双向') {
		duplex = 2;
	}
  let frame_duration = 10000;
  let rtn_num = master_rtn.val;//role.val == '主机' ? master_rtn.val : slave_rtn.val;
  if (role.val == '从机') {
  	rtn_num = slave_rtn.val;
  }
  let mtl_num = master_mtl.val;//role.val == '主机' ? master_mtl.val : slave_mtl.val;
  if (role.val == '从机') {
  	mtl_num = slave_mtl.val;
  }
  if (frame_dms.val == '2.5ms') {
  	frame_duration = 2500;
  } else if (frame_dms.val == '5ms') {
  	frame_duration = 5000;
  } else if (frame_dms.val == '7.5ms') {
  	frame_duration = 7500;		
  } else if (frame_dms.val == '10ms') {
  	frame_duration = 10000;
  }
  let frame_num = 1000000 / frame_duration;
  let packet_num = 1000000 / sdu_period.val;
  let real_bitrate = bitrate.val + (frame_num * 2 * 8) + (packet_num * 11 * 8);
  let sync_delay = (((real_bitrate / 1000) * (frame_duration / 100) + (2 + 4) * 80) / 2 + 1500 + 440) * (rtn_num + 1) * duplex * cis_channel / 10;
  let enc_time = frame_duration / 2;
  //TODO tx delay需要加上 --蓝牙
 	latency.min = 1000 + enc_time + sync_delay + sdu_period.val + mtl_num * sdu_period.val;
  if(latency.val < latency.min){
    latency.val = latency.min;
    show('CIS MIC输入模式延时应根据发包间隔、重发、预发包和码率设置', true)
  }
}

if (src.name == 'BIS功能配置.接收使能' && !src.val){
	let a= find("$BIS配置.BIS功能配置.本地同步播放")
	let c = find("$BIS配置.BIS功能配置.发送使能")
	let d = find("$BIS配置.BIS功能配置.广播角色")
	let e = find("$蓝牙配置.LE_AUDIO配置.le_audio 应用选择")
	let f = find("$BIS配置.BIS功能配置.接收端关EDR")
		
	f.val = 0;
	if(!(e.val.find(v=>v==='LE_AUDIO_AURACAST_SOURCE_EN')) && !(e.val.find(v=>v==='LE_AUDIO_AURACAST_SINK_EN')) && !c.val){
		d.val = '不固定';
		a.val = 1;
		show('广播使能关闭，自动固定为不固定端角色')
	}else if (d.val != '发送端' && c.val){
		d.val = '发送端';
		a.val = 1;
		show('只开发送使能，自动固定为发送端角色',true)
	}
}

if (src.name == 'BIS功能配置.接收使能' && src.val) {
	let a= find("$BIS配置.BIS功能配置.本地同步播放")
	let c = find("$BIS配置.BIS功能配置.发送使能")
	let d = find("$BIS配置.BIS功能配置.广播角色")
	let e = find("$BIS配置.BIS功能配置.接收端关EDR")
	
	if (d.val != '接收端' && !c.val){
		d.val = '接收端';
		a.val = 0;
		e.val = 1;
		show('只开接收使能，自动固定为接收端角色',true)
	}
}

if (src.name == 'BIS功能配置.发送使能' && !src.val) {
	let a= find("$BIS配置.BIS功能配置.本地同步播放")
	let c = find("$BIS配置.BIS功能配置.接收使能")
	let d = find("$BIS配置.BIS功能配置.广播角色")
	let e = find("$蓝牙配置.LE_AUDIO配置.le_audio 应用选择")
	let f = find("$BIS配置.BIS功能配置.接收端关EDR")
	
	if(!(e.val.find(v=>v==='LE_AUDIO_AURACAST_SOURCE_EN')) && !(e.val.find(v=>v==='LE_AUDIO_AURACAST_SINK_EN')) && !c.val){
		d.val = '不固定';
		a.val = 1;
		show('广播使能关闭，自动固定为不固定端角色')
	}else if (d.val != '接收端' && c.val){
		d.val = '接收端';
		a.val = 0;
		f.val = 1;
		show('只开接收使能，自动固定为接收端角色',true)
	}
}

if (src.name == 'BIS功能配置.发送使能' && src.val) {
	let a = find("$BIS配置.BIS功能配置.本地同步播放")
	let c = find("$BIS配置.BIS功能配置.接收使能")
	let d = find("$BIS配置.BIS功能配置.广播角色")
	if (d.val != '发送端' && !c.val){
		d.val = '发送端';
		a.val = 1;
		show('只开发送使能，自动固定为发送端角色',true)
	}
}

if (src.name == 'BIS功能配置.广播角色' && src.val == '不固定') {
	
	let a = find("$BIS配置.BIS功能配置.广播角色")
	let b = find("$BIS配置.BIS功能配置.本地同步播放")
	let c = find("$BIS配置.BIS功能配置.接收使能")
	let d = find("$BIS配置.BIS功能配置.发送使能")
	let e = find("$BIS配置.BIS功能配置.接收端关EDR")
	
	e.val = 0;
	if ( !(c.val && d.val) ){
		if (c.val){
		  show('只开接收使能，自动固定为接收端角色',true)
		  a.val = '接收端';
		  b.val = 0;
		  e.val = 1;
		}else if(d.val){
		  show('只开发送使能，自动固定为发送端角色',true)		
		  a.val = '发送端';
		  b.val = 1;
		}
		
	}else if (!c.val && !d.val){
		c = find("$蓝牙配置.LE_AUDIO配置.le_audio 应用选择")
		if ((c.val.find(v=>v==='LE_AUDIO_AURACAST_SINK_EN')) && !(c.val.find(v=>v==='LE_AUDIO_AURACAST_SOURCE_EN'))){
		  show('只开接收使能，自动固定为接收端角色',true)
		  a.val = '接收端';
		  b.val = 0;
		  e.val = 1;
		}else if ((c.val.find(v=>v==='LE_AUDIO_AURACAST_SOURCE_EN')) && !(c.val.find(v=>v==='LE_AUDIO_AURACAST_SINK_EN'))){
		  show('只开发送使能，自动固定为发送端角色',true)
		  a.val = '发送端';
		  b.val = 1;
		}
		
		
	}
}

if (src.name == 'BIS功能配置.广播角色' && src.val == '接收端') {
	
	let a = find("$BIS配置.BIS功能配置.广播角色")
	let b = find("$BIS配置.BIS功能配置.本地同步播放")
	let c = find("$BIS配置.BIS功能配置.接收使能")
	let d = find("$BIS配置.BIS功能配置.发送使能")
	let e = find("$BIS配置.BIS功能配置.接收端关EDR")
	
	e.val = 1;
	if (!c.val && d.val){
	  show('只开发送使能，自动固定为发送端角色',true)
	  a.val = '发送端';
	  b.val = 1;
	}else if (!c.val && !d.val){
	  c = find("$蓝牙配置.LE_AUDIO配置.le_audio 应用选择")
	  if ((c.val.find(v=>v==='LE_AUDIO_AURACAST_SOURCE_EN')) && !(c.val.find(v=>v==='LE_AUDIO_AURACAST_SINK_EN'))){
		show('只开发送使能，自动固定为发送端角色',true)
		a.val = '发送端';
		b.val = 1;
	  }else if(!(c.val.find(v=>v==='LE_AUDIO_AURACAST_SINK_EN')) && !(c.val.find(v=>v==='LE_AUDIO_AURACAST_SINK_EN'))){
		a.val = '不固定';
		b.val = 1;
		show('广播使能关闭，自动固定为不固定端角色')
	  }
		
		
	}

}

if (src.name == 'BIS功能配置.广播角色' && src.val == '发送端') {
	
	let a = find("$BIS配置.BIS功能配置.广播角色")
	let b = find("$BIS配置.BIS功能配置.本地同步播放")
	let c = find("$BIS配置.BIS功能配置.接收使能")
	let d = find("$BIS配置.BIS功能配置.发送使能")
	let e = find("$BIS配置.BIS功能配置.接收端关EDR")
	
	e.val =0;
	if (!d.val && c.val){
	  show('只开接收使能，自动固定为接收端角色',true)
	  a.val = '接收端';
	  b.val = 0;
	  e.val = 1;
	}else if (!c.val && !d.val){
	  c = find("$蓝牙配置.LE_AUDIO配置.le_audio 应用选择")
	  if ((c.val.find(v=>v==='LE_AUDIO_AURACAST_SINK_EN')) && !(c.val.find(v=>v==='LE_AUDIO_AURACAST_SOURCE_EN'))){
		show('只开接收使能，自动固定为接收端角色',true)
		a.val = '接收端';
		b.val = 0;
		e.val = 1;
	  }else if(!(c.val.find(v=>v==='LE_AUDIO_AURACAST_SOURCE_EN')) && !(c.val.find(v=>v==='LE_AUDIO_AURACAST_SINK_EN'))){
		a.val = '不固定';
		b.val = 1;
		show('广播使能关闭，自动固定为不固定端角色')
	  }
	  
	}

}




if (src.name == 'LE_AUDIO配置.le_audio 应用选择' && (src.val.find(v=>v==='LE_AUDIO_AURACAST_SINK_EN'))){
	
	let a = find("$BIS配置.BIS功能配置.广播角色")

	let b = find("$BIS配置.BIS功能配置.本地同步播放")
	

	if (src.val.find(v=>v==='LE_AUDIO_AURACAST_SOURCE_EN')){
		show('开启AURACAST，请关闭BIS发送与接收使能，CIS主机与从机使能，注意查看广播是否固定角色')
	}else if (a.val != '接收端'){
		a.val = '接收端';
		b.val = 0;
		show('开启AURACAST，请关闭BIS发送与接收使能，CIS主机与从机使能。当前只开接收使能，自动固定为接收端角色')
	}
	
}

if (src.name == 'LE_AUDIO配置.le_audio 应用选择' && (src.val.find(v=>v==='LE_AUDIO_AURACAST_SOURCE_EN'))){
	
	let a = find("$BIS配置.BIS功能配置.广播角色")

	let b = find("$BIS配置.BIS功能配置.本地同步播放")
	

	if (src.val.find(v=>v==='LE_AUDIO_AURACAST_SINK_EN')){
		show('开启AURACAST，请关闭BIS发送与接收使能，CIS主机与从机使能，注意查看广播是否固定角色')
	}else if (a.val != '发送端'){
		a.val = '发送端';
		b.val = 1;
		show('开启AURACAST，请关闭BIS发送与接收使能，CIS主机与从机使能。当前只开发送使能，自动固定为发送端角色')
	}
	
}

if (src.name == 'LE_AUDIO配置.le_audio 应用选择' && !(src.val.find(v=>v==='LE_AUDIO_AURACAST_SOURCE_EN'))){
	
	let a = find("$BIS配置.BIS功能配置.广播角色")

	let b = find("$BIS配置.BIS功能配置.本地同步播放")
	
	let c = find("$BIS配置.BIS功能配置.接收使能")
	let d = find("$BIS配置.BIS功能配置.发送使能")

    if(!(src.val.find(v=>v==='LE_AUDIO_AURACAST_SINK_EN')) && !c.val && !d.val){
		a.val = '不固定';
		b.val = 1;
		show('广播使能关闭，自动固定为不固定端角色')
	}else if ((src.val.find(v=>v==='LE_AUDIO_AURACAST_SINK_EN')) && (a.val != '接收端')){
		a.val = '接收端';
		b.val = 0;
		show('当前只开接收使能，自动固定为接收端角色')
	}
	
}

if (src.name == 'LE_AUDIO配置.le_audio 应用选择' && !(src.val.find(v=>v==='LE_AUDIO_AURACAST_SINK_EN'))){
	
	let a = find("$BIS配置.BIS功能配置.广播角色")

	let b = find("$BIS配置.BIS功能配置.本地同步播放")
	
	let c = find("$BIS配置.BIS功能配置.接收使能")
	let d = find("$BIS配置.BIS功能配置.发送使能")
	if(!(src.val.find(v=>v==='LE_AUDIO_AURACAST_SOURCE_EN')) && !c.val && !d.val){
		a.val = '不固定';
		b.val = 1;
		show('广播使能关闭，自动固定为不固定端角色')
	}else if ((src.val.find(v=>v==='LE_AUDIO_AURACAST_SOURCE_EN')) && (a.val != '发送端')){
		a.val = '发送端';
		b.val = 1;
		show('当前只开发送使能，自动固定为发送端角色')
	}
	
}


if((src.name == '经典蓝牙.LHDC_V3/V4' || src.name == '经典蓝牙.LHDC_V5') && src.val == 1) {
  show('打开LHDC，若流程图为localTws，需要在switch节点后添加播放同步节点')
}

if(src.name == 'BLE' && src.groupEnable == 1){
	show('单独使能BLE无法搜索到BLE广播, 需要配合第三方协议配置')
}


//-------------------------Pingpon Echo Delay ---------------------------
if (src.name == 'Pingpong Echo.Delay') {
    const dest = find('Pingpong Echo.Max Delay');
    if (dest) {
        src.max = dest.val;
        if (src.val > dest.val) {
            src.val = dest.val;
        }
    }
}
if (src.name == 'Pingpong Echo.Pre Delay') {
    const dest = find('Pingpong Echo.Max Delay');
    if (dest) {
        src.max = dest.val;
        if (src.val > dest.val) {
            src.val = dest.val;
        }
    }
}
if (src.name == 'Pingpong Echo.Max Delay') {
    const dest = find('Pingpong Echo.Delay');
    const dest2 = find('Pingpong Echo.Pre Delay');
    if (dest) {
        if (src.val < dest.val) {
            dest.val = src.val;
            dest.max = src.val;
        }
    }
    if (dest2) {
        if (src.val < dest2.val) {
            dest2.val = src.val;
            dest2.max = src.val;
        }
    }
}

