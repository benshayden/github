const MAIL = 'https://mail.google.com/';
const MEET = 'https://meet.google.com';

async function updateWin(w, d) {
  try {
    await chrome.windows.update(w.id, d);
    console.log('update success', d, w);
  } catch (e) {
    console.error('update failed', d, w, e);
  }
}

function makeItSo() {
  chrome.system.display.getInfo({}, async function(info) {
    console.log('display.getInfo', info);
    const biDisp = {state: 'normal', ...info.filter(d => d.name === 'Built-in display')[0]?.workArea};
    const extDisp = info.filter(d => d.name !== 'Built-in display')[0]?.workArea;
    chrome.windows.getAll({populate: true}, async function(windows) {
      console.log('windows.getAll', windows);
      const tile = {
        width: Math.floor(extDisp.width / 3), 
        height: Math.floor(extDisp.height / 2),
      };
      let x = 0, y = 0;
      const meet = windows.filter(w => w.tabs.filter(t => t.url.startsWith(MEET)).length > 0)[0];
      if (extDisp && meet) {
        console.log('found meet', meet);
        await updateWin(meet, {
          state: 'normal',
          ...tile,
          left: extDisp.left + tile.width,
          top: extDisp.top + tile.height,
        });
      }

      for (const w of windows) {
        if (!extDisp || w.tabs.filter(t => t.url.startsWith(MAIL)).length > 0) {
          console.log('found mail', w);
          await updateWin(w, biDisp);
          continue;
        }
        if (extDisp && w === meet) {
          continue;
        }
        console.log('tiling', {x, y}, w);
        await updateWin(w, y >= 2 ? biDisp : {
          state: 'normal',
          ...tile,
          left: extDisp.left + tile.width * x,
          top: extDisp.top + tile.height * y,
        });
        x += 1;
        if (x === 3) {
          x = 0;
          y += 1;
        }
        if (meet && y == 1 && x == 1) {
          x += 1;
        }
      }
    });
  });
}

chrome.windows.onCreated.addListener(async function(w) {
  console.log('onCreated', w);
  makeItSo();
});

chrome.system.display.onDisplayChanged.addListener(async function() {
  console.log('display.onDisplayChanged');
  makeItSo();
});

makeItSo();
