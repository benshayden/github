var IDE = 'http://openjscad.org/';

chrome.tabs.onUpdated.addListener(function(tabId, delta, tab) {
  if (tab.url.match('^https:\/\/github\.com\/[^/]+/[^/]+\/edit\/master\/.*\.jscad$')) {
    chrome.pageAction.show(tabId);
  } else {
    chrome.pageAction.hide(tabId);
  }
});

function getgit(tid) {
  chrome.runtime.sendMessage({tid: tid, url: location.href, gitsrc: $('.ace_editor')[0].env.editor.session.getValue()});
}

chrome.pageAction.onClicked.addListener(function(tab) {
  chrome.tabs.executeScript({code: '(' + getgit + ')(' + tab.id + ')'});
});

function setide(s, u) {
  putSourceInEditor(s);
  gProcessor.setJsCad(s);
  gProcessor.onchange=function() {
    if (gProcessor.errorpre.innerText) return;
    chrome.runtime.sendMessage({giturl: u, idesrc: editor.session.getValue()});
  };
}

function setgit(s) {
  $('.ace_editor')[0].env.editor.session.setValue(s);
  $('#submit-file')[0].click();
  window.close();
}

chrome.runtime.onMessage.addListener(function(request, sender, sendResponse) {
  if (request.gitsrc) {
    // Close the github/edit tab so the user doesn't accidentally edit the file
    // in two places. When we want to commit changes, we'll re-open the tab,
    // commit, and close it again instead of clicking the edit button.
    chrome.tabs.remove(request.tid);
    chrome.tabs.create({active: true, url: IDE}, function(tab) {
      chrome.tabs.executeScript({code: '(' + setide + ')(' + JSON.stringify(request.gitsrc) + ', ' + JSON.stringify(request.url) + ')'})
    });
  } else if (request.giturl && request.idesrc) {
    chrome.tabs.create({active: false, url: request.giturl}, function(tab) {
      chrome.tabs.executeScript(tab.id, {code: '(' + setgit + ')(' + JSON.stringify(request.idesrc) + ')'})
    });
  }
});
