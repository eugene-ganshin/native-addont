const addon = require('bindings')('greet')
exports.hello = addon.greetHello

