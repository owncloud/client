/**
 * @enum {string}
 * @readonly
 */
const BACKENDS = (exports.BACKENDS = Object.freeze({
  local: 'LOCAL',
  remote: 'REMOTE'
}))

/**
 * Give the backend URL for currently default backend
 */
exports.getCurrentBackendUrl = function() {
  // eslint-disable-next-line camelcase
  // const { backend_url, remote_backend_url, default_backend } = client.globals
  // eslint-disable-next-line camelcase
  // return default_backend === BACKENDS.local ? backend_url : remote_backend_url
  return process.env.BACKEND_HOST
}
