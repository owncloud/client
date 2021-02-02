const userSettings = require('../helpers/userSettings')
const _ = require('lodash')
const fetch = require('node-fetch')
const { join } = require('./path')
const backendHelper = require('../helpers/backendHelper')

/**
 *
 * @param {string} userId
 *
 * @returns {{Authorization: string}}
 */
const createAuthHeader = function(userId) {
  const password = userSettings.getPasswordForUser(userId)
  return {
    Authorization: 'Basic ' + Buffer.from(userId + ':' + password).toString('base64')
  }
}
/**
 *
 * @param {string} userId
 *
 * @returns {{<header>: string}}
 */
const createOCSRequestHeaders = function(userId) {
  return {
    ...createAuthHeader(userId),
    'OCS-APIREQUEST': true
  }
}
/**
 *
 * @param {node-fetch.Response} response
 * @param {string} message
 *
 * @throws Error
 * @returns {node-fetch.Response}
 */
const checkStatus = function(response, message = '') {
  if (response.ok) {
    // response.status >= 200 && response.status < 300
    return response
  } else {
    throw Error(
      `HTTP Request Failed: ${message}, Status: ${response.status} ${response.statusCode}`
    )
  }
}

/**
 *
 * @param {Object} response the response body in json
 * @param {string} message
 *
 * @throws Error
 * @returns {Object} the body of the response
 */
const checkOCSStatus = function(response, message = '') {
  const statusCode = _.get(response, 'ocs.meta.statuscode')
  const ocsMessage = _.get(response, 'ocs.meta.message')
  if (statusCode === 200) {
    return response
  } else {
    throw Error(`OCS Request Failed: ${message}, Status: ${statusCode}, Message: ${ocsMessage}`)
  }
}

/**
 *
 * @param {string} url
 * @param {object} options
 *
 * @returns {node-fetch}
 */
const fetcher = (url, options) => fetch(url, options)

/**
 *
 * @param {string} path
 * @param {object} params
 * @param {string} userId
 * @param {object} header
 *
 * @returns {node-fetch}
 */
const requestEndpoint = function(path, params, userId = 'admin', header = {}) {
  const headers = { ...createAuthHeader(userId), ...header }
  const options = { ...params, headers }
  const url = join(backendHelper.getCurrentBackendUrl(), 'remote.php/dav', path)
  return fetcher(url, options)
}

/**
 *
 * @param {string} path
 * @param {object} params
 * @param {string} userId
 * @param {object} header
 *
 * @returns {node-fetch}
 */
const requestOCSEndpoint = function(path, params, userId = 'admin', header = {}) {
  const headers = { ...createOCSRequestHeaders(userId), ...header }
  const options = { ...params, headers }
  const separator = path.includes('?') ? '&' : '?'
  const url = join(
    backendHelper.getCurrentBackendUrl(),
    'ocs/v2.php',
    path + separator + 'format=json'
  )
  return fetcher(url, options)
}

module.exports = {
  createAuthHeader,
  createOCSRequestHeaders,
  checkStatus,
  checkOCSStatus,
  requestEndpoint,
  requestOCSEndpoint,
  // ocs request methods
  getOCS: (url, userId, body, header) =>
    requestOCSEndpoint(url, { body, method: 'GET' }, userId, header),
  putOCS: (url, userId, body, header) =>
    requestOCSEndpoint(url, { body, method: 'PUT' }, userId, header),
  postOCS: (url, userId, body, header) =>
    requestOCSEndpoint(url, { body, method: 'POST' }, userId, header),
  deleteOCS: (url, userId, body, header) =>
    requestOCSEndpoint(url, { body, method: 'DELETE' }, userId, header),
  // dav request methods
  get: (url, userId, body, header) => requestEndpoint(url, { body, method: 'GET' }, userId, header),
  put: (url, userId, body, header) => requestEndpoint(url, { body, method: 'PUT' }, userId, header),
  delete: (url, userId, body, header) =>
    requestEndpoint(url, { body, method: 'DELETE' }, userId, header),
  move: (url, userId, body, header) =>
    requestEndpoint(url, { body, method: 'MOVE' }, userId, header),
  mkcol: (url, userId, body, header) =>
    requestEndpoint(url, { body, method: 'MKCOL' }, userId, header),
  propfind: (url, userId, body, header) =>
    requestEndpoint(url, { body, method: 'PROPFIND' }, userId, header),
  report: (url, userId, body, header) =>
    requestEndpoint(url, { body, method: 'REPORT' }, userId, header),
  proppatch: (url, userId, body, header) =>
    requestEndpoint(url, { body, method: 'PROPPATCH' }, userId, header)
}
