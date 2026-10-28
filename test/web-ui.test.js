const assert = require('node:assert/strict');
const fs = require('node:fs');
const test = require('node:test');
const ui = require('../data/www/app.js');

test('maps controlled HTTP failures to safe user-facing messages', () => {
  assert.match(ui.messageForHttpFailure(403), /Authorization failed/);
  assert.match(ui.messageForHttpFailure(429), /Too many failed/);
  assert.match(ui.messageForHttpFailure(400), /Invalid network configuration/);
  assert.match(ui.messageForHttpFailure(413), /too large/);
  assert.match(ui.messageForHttpFailure(500), /could not save/);
  assert.match(ui.messageForHttpFailure(403, 'provisioning_inactive'), /no longer active/);
  assert.match(ui.messageForHttpFailure(400, 'missing_device_name'), /device name/);
  assert.match(ui.messageForHttpFailure(400, 'wifi_ssid_too_long'), /32 bytes/);
  assert.match(ui.messageForHttpFailure(400, 'malformed_form'), /Reload/);
  assert.doesNotMatch(ui.messageForHttpFailure(500), /password|secret/i);
});

test('maps timeout and connection loss without exposing response text', () => {
  assert.match(ui.errorMessage({ type: 'timeout' }), /did not respond in time/);
  assert.match(ui.errorMessage({ type: 'network' }), /interrupted/);
});

test('only a device-reported connected state confirms Wi-Fi association', () => {
  assert.equal(ui.connectionProgressFor('provisioning'), 'Ready to configure');
  assert.equal(ui.connectionProgressFor('unconfigured'), 'Wi-Fi has not been configured');
  assert.equal(ui.connectionProgressFor('connecting'), 'Waiting for device connection');
  assert.equal(ui.connectionProgressFor('connected'), 'Connection confirmed');
  assert.equal(ui.connectionProgressFor('reconnecting'), 'Connection could not be verified');
  assert.equal(ui.connectionProgressFor('cooling_down'), 'Connection retry is cooling down');
  assert.equal(ui.connectionProgressFor('failed'), 'Connection attempt failed');
});

test('uses UTF-8 byte limits that match firmware validation', () => {
  assert.equal(ui.byteLength('é'), 2);
  assert.equal(ui.validateNetworkValues(new Map([['name', 'Tally'], ['ssid', 'studio'], ['password', 'p'.repeat(64)]])), null);
  assert.equal(ui.validateNetworkValues(new Map([['name', 'é'.repeat(17)], ['ssid', 'studio'], ['password', '']])), 'name');
});

test('maximum valid URL-encoded credentials fit the bounded request size', () => {
  const body = new URLSearchParams({ name: '!'.repeat(32), ssid: '!'.repeat(32), password: '!'.repeat(64) });
  assert.equal(body.toString().includes('------'), false);
  assert.ok(body.toString().length <= 416);
  assert.equal(body.toString().length, 405);
});

test('Safari-compatible serialization retains a manually entered device name', () => {
  const form = ui.encodeNetworkForm(new Map([
    ['name', 'Studio Tally'],
    ['ssid', 'Studio Wi-Fi'],
    ['password', 'safe password']
  ]));
  assert.equal(form.get('name'), 'Studio Tally');
  assert.equal(form.toString(), 'name=Studio+Tally&ssid=Studio+Wi-Fi&password=safe+password');
  assert.deepEqual([...form.keys()], ['name', 'ssid', 'password']);
  assert.match('application/x-www-form-urlencoded;charset=UTF-8', /^application\/x-www-form-urlencoded/);
});

test('preserves spaces and special characters in Safari-style network forms', () => {
  const ssidWithSpaces = 'Elias iPhone 14 pro';
  const passwordWithSpecialCharacters = 'S@fe +$%&=;!';
  const form = new URLSearchParams({ name: 'Tally', ssid: ssidWithSpaces, password: passwordWithSpecialCharacters });
  assert.equal(form.toString(),
               'name=Tally&ssid=Elias+iPhone+14+pro&password=S%40fe+%2B%24%25%26%3D%3B%21');
  assert.equal(new URLSearchParams(form).get('ssid'), ssidWithSpaces);
  assert.equal(new URLSearchParams(form).get('password'), passwordWithSpecialCharacters);
  assert.equal(new URLSearchParams({ ssid: 'Studio' }).get('ssid'), 'Studio');
});

test('station connection passes configured SSID and password buffers unchanged', () => {
  const controller = fs.readFileSync('src/network/NetworkController.cpp', 'utf8');
  assert.match(controller, /WiFi\.begin\(configuration_\.wifiSsid, configuration_\.wifiPassword\)/);
  assert.doesNotMatch(controller, /wifiSsid\.trim\(|wifiPassword\.trim\(/);
});

test('provisioning page uses a versioned URL-encoded form asset', () => {
  const page = fs.readFileSync('data/www/index.html', 'utf8');
  const script = fs.readFileSync('data/www/app.js', 'utf8');
  assert.match(page, /app\.js\?v=20261028-network-form-capture/);
  assert.match(script, /const requestBody = encodeNetworkForm\(values\);/);
  assert.ok(script.indexOf('const requestBody = encodeNetworkForm(values);') <
            script.indexOf('setNetworkFormPending(true);'));
  assert.doesNotMatch(script, /new FormData\(event\.target\)/);
});
