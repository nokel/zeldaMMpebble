var Clay = require('@rebble/clay');
var clayConfig = require('./config');

// Clay handles showConfiguration and webviewclosed automatically.
// When the user closes the config page, Clay sends all messageKeys
// to the watch via AppMessage.
var clay = new Clay(clayConfig);
