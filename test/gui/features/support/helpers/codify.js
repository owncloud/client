const _ = require('lodash')

const { passwords } = require('./userSettings')
const backendHelper = require('./backendHelper')

/**
 * Gets the password for the un-initialized users and replaces the password accordingly
 *
 * @param {string} input
 * @returns {string}
 */
exports.replaceInlineCode = function(input) {
  const codes = {
    ...passwords,
    local_server: backendHelper.getCurrentBackendUrl(),
    client_sync_path: process.env.CLIENT_SYNC_PATH,
    local_server_hostname: new URL(backendHelper.getCurrentBackendUrl()).hostname
  }
  const interpolate = /%([\s\S]+?)%/g
  const compiled = _.template(input, { interpolate })
  return compiled(codes)
}

exports.replaceInlineTable = function(dataTable) {
  dataTable.raw().forEach(row => {
    row.forEach((cell, index, arr) => {
      arr[index] = exports.replaceInlineCode(cell)
    })
  })
  return dataTable
}
