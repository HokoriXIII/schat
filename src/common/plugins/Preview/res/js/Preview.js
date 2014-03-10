(function() {
  'use strict';

  var cache = {};

  /**
   * Обновление состояния элемента.
   *
   * @param item
   * @param elem
   */
  function update(item, elem) {
    if (typeof item === 'string')
      item = PreviewPlugin.findById(item);

    if (item === null || item.state === 2) {
      elem.setAttribute('class', 'img-thumbnail img-thumbnail-error');
      return;
    }

    if (item.state === 0) {
      elem.setAttribute('class', 'img-thumbnail img-thumbnail-spinner');
      return;
    }

    if (item.state === 1) {
      elem.setAttribute('class', 'img-thumbnail');

      if (item.flags & 1) {
        elem.style.backgroundImage = 'url("' + item.orig + '")';
        elem.style.backgroundSize  = (item.width > 90 ? 90 : item.width) + 'px ' + (item.height > 90 ? 90 : item.height) + 'px';
      }
      else {
        elem.style.backgroundImage = 'url("' + item.thumb + '")';
      }
    }
  }


  /**
   * Слот для обработки изменения состояния сообщения.
   *
   * @param id
   */
  function onChanged(id) {
    if (!cache.hasOwnProperty(id))
      return;

    var list = cache[id];
    var item = PreviewPlugin.findById(id);

    for (var i = 0; i < list.length; ++i) {
      update(item, list[i]);
    }
  }


  /**
   * Обработка добавления нового сообщения.
   *
   * @param id Идентификатор сообщения.
   */
  function onAdd(id) {
    var images = PreviewPlugin.findByOID(id);
    if (images === null)
      return;

    var container = document.getElementById(id);
    if (container === null)
      return;

    container.setAttribute('data-remover', 'preview');

    var block = document.createElement('div');
    block.setAttribute('class', 'thumbnail-block');

    for (var i = 0; i < images.length; ++i) {
      id = images[i];

      var a = document.createElement('a');

      var item = PreviewPlugin.findById(id);
      if (item !== null) {
        a.setAttribute('href',  item.url);
        a.setAttribute('title', item.url);
      }

      update(item, a);

      cache[id] = cache[id] || [];
      cache[id].push(a);

      block.appendChild(a);
    }

    container.firstChild.appendChild(block);
  }


  Messages.previewRemover = function(container) {
    Messages.genericRemover(container);

    var block = container.firstChild;
    block.removeChild(block.children[3]);
  };


  Messages.onAdd.push(onAdd);

  if (typeof window.PreviewPlugin === 'undefined') {
    window.PreviewPlugin = {
      findById: function(id)  { return null; },
      findByOID: function(id) { return null; }
    }
  }
  else {
    PreviewPlugin.changed.connect(onChanged);
  }
})();