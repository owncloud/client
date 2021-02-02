// const { client } = require('nightwatch-api')
const httpHelper = require('../helpers/httpHelper')
const backendHelper = require('./backendHelper')
const convert = require('xml-js')
const _ = require('lodash/object')
const { normalize, join, filename } = require('../helpers/path')
const occHelper = require('../helpers/occHelper')

const uploadTimeStamps = {}
const deleteTimestamps = {}

/**
 *
 * @param {string} userId
 * @param {string} element
 * @param {string} server - (REMOTE/LOCAL) server to use for dav path
 */
exports.createDavPath = function(userId, element) {
  const replaceEncoded = encodeURIComponent(element).replace(/%2F/g, '/')
  return `files/${userId}/${replaceEncoded}`
}

/**
 *
 * @param {string} userId
 * @param {string} file
 */
exports.download = function(userId, file) {
  const davPath = exports.createDavPath(userId, file)
  return httpHelper
    .get(davPath, userId)
    .then(res => httpHelper.checkStatus(res, 'Could not download file.'))
    .then(res => res.text())
}

/**
 *
 * @param {string} userId
 * @param {string} file
 */
exports.delete = async function(userId, file) {
  const davPath = exports.createDavPath(userId, file)

  const filename = file.split('/')[file.split('/').length - 1]

  /**
   * makes sure delete operations are carried out maximum once a second to avoid trashbin issues
   * see https://github.com/owncloud/core/issues/23151
   */
  if (deleteTimestamps[userId] && deleteTimestamps[userId][filename]) {
    const timeSinceLastDelete = Date.now() - deleteTimestamps[userId][filename]
    if (timeSinceLastDelete < 1001) {
      // await client.pause(1001 - timeSinceLastDelete)
    }
  }

  return httpHelper
    .delete(davPath, userId)
    .then(function(res) {
      deleteTimestamps[userId] = deleteTimestamps[userId] || {}
      deleteTimestamps[userId][filename] = Date.now()
      return httpHelper.checkStatus(res, 'Could not delete file ' + file)
    })
    .then(res => res.text())
}

/**
 * Rename a file using webDAV api.
 *
 * @param {string} userId
 * @param {string} fromName
 * @param {string} toName
 */
exports.move = function(userId, fromName, toName) {
  const davPath = exports.createDavPath(userId, fromName)
  let body
  return httpHelper
    .move(davPath, userId, body, {
      Destination: join(
        backendHelper.getCurrentBackendUrl(),
        '/remote.php/dav/',
        exports.createDavPath(userId, toName)
      )
    })
    .then(res => httpHelper.checkStatus(res, 'Could not move file.'))
    .then(res => res.text())
}

/**
 *
 * @param {string} path
 * @param {string} userId
 * @param {array} properties
 * @param {number} folderDepth
 */
exports.propfind = function(path, userId, properties, folderDepth = '1') {
  const davPath = encodeURI(path)
  let propertyBody = ''
  properties.map(prop => {
    propertyBody += `<${prop}/>`
  })
  const body = `<?xml version="1.0"?>
                <d:propfind
                xmlns:d="DAV:"
                xmlns:oc="http://owncloud.org/ns"
                xmlns:ocs="http://open-collaboration-services.org/ns">
                <d:prop>${propertyBody}</d:prop>
                </d:propfind>`
  return httpHelper.propfind(davPath, userId, body, { Depth: folderDepth }).then(res => res.text())
}

/**
 * Get the list of trashbin items for a user
 * in following format
 * [{
 *  "href":
 *  "originalFilename":
 *  "originalLocation":
 *  "deleteTimestamp":
 *  "lastModified":
 * },...]
 *
 * @param {string} user
 */
exports.getTrashBinElements = function(user, depth = 2) {
  return new Promise((resolve, reject) => {
    exports
      .propfind(
        `/trash-bin/${user}`,
        user,
        [
          'oc:trashbin-original-filename',
          'oc:trashbin-original-location',
          'oc:trashbin-delete-timestamp',
          'd:getlastmodified'
        ],
        depth
      )
      .then(str => {
        const trashData = convert.xml2js(str, { compact: true })['d:multistatus']['d:response']
        const trashItems = []
        trashData.map(trash => {
          if (trash['d:propstat']['d:prop'] === undefined) {
            reject(new Error('trashbin data not defined'))
          } else {
            trashItems.push({
              href: trash['d:href']._text,
              originalFilename:
                trash['d:propstat']['d:prop']['oc:trashbin-original-filename']._text,
              originalLocation:
                trash['d:propstat']['d:prop']['oc:trashbin-original-location']._text,
              deleteTimestamp: trash['d:propstat']['d:prop']['oc:trashbin-delete-timestamp']._text,
              lastModified: trash['d:propstat']['d:prop']['d:getlastmodified']._text
            })
          }
        })
        resolve(trashItems)
      })
  })
}

/**
 * Create a folder using webDAV api.
 *
 * @param {string} user
 * @param {string} folderName
 */
exports.createFolder = function(user, folderName) {
  const davPath = exports.createDavPath(user, folderName)
  return httpHelper
    .mkcol(davPath, user)
    .then(res =>
      httpHelper.checkStatus(res, `Could not create the folder "${folderName}" for user "${user}".`)
    )
    .then(res => res.text())
}
/**
 * Create a file using webDAV api.
 *
 * @param {string} user
 * @param {string} fileName
 * @param {string} contents
 */
exports.createFile = async function(user, fileName, contents = '') {
  const davPath = exports.createDavPath(user, fileName)
  /**
   * makes sure upload operations are carried out maximum once a second to avoid version issues
   * see https://github.com/owncloud/core/issues/23151
   */

  if (uploadTimeStamps[user] && uploadTimeStamps[user][fileName]) {
    const timeSinceLastFileUpload = Date.now() - uploadTimeStamps[user][fileName]
    if (timeSinceLastFileUpload <= 1001) {
      // await client.pause(1001 - timeSinceLastFileUpload)
    }
  }

  return httpHelper
    .put(davPath, user, contents)
    .then(function(res) {
      uploadTimeStamps[user] = uploadTimeStamps[user] || {}
      uploadTimeStamps[user][fileName] = Date.now()
      return httpHelper.checkStatus(
        res,
        `Could not create the file "${fileName}" for user "${user}".`
      )
    })
    .then(res => res.text())
}

/**
 * Get file or folder properties using webDAV api.
 *
 * @param {string} path
 * @param {string} userId
 * @param {array} requestedProps
 */
exports.getProperties = function(path, userId, requestedProps) {
  return new Promise((resolve, reject) => {
    const trimmedPath = normalize(path) // remove a leading slash
    const relativePath = `/files/${userId}/${trimmedPath}`
    exports.propfind(relativePath, userId, requestedProps, 0).then(str => {
      const response = JSON.parse(convert.xml2json(str, { compact: true }))
      const receivedProps = _.get(
        response,
        "['d:multistatus']['d:response']['d:propstat']['d:prop']"
      )
      if (receivedProps === undefined) {
        const errMsg = "Could not find 'd:prop' inside response. Received:\n"
        return reject(new Error(errMsg + JSON.stringify(str)))
      }
      const properties = {}
      requestedProps.map(propertyName => {
        properties[propertyName] = receivedProps[propertyName]._text
      })
      return resolve(properties)
    })
  })
}

exports.getSkeletonFile = function(filename) {
  return occHelper
    .runOcc(['config:system:get', 'skeletondirectory'])
    .then(resp => resp.ocs.data.stdOut)
    .then(dir => {
      const element = join(dir.trim(), filename)
      const apiURL = `apps/testing/api/v1/file?file=${encodeURIComponent(element)}&absolute=true`
      return httpHelper.getOCS(apiURL, 'admin')
    })
    .then(res => res.json())
    .then(body => {
      return decodeURIComponent(body.ocs.data[0].contentUrlEncoded)
    })
}

exports.getFavouritedResources = function(user) {
  const body = `<oc:filter-files  xmlns:d="DAV:" xmlns:oc="http://owncloud.org/ns">
                 <d:prop><d:resourcetype /></d:prop>
                 <oc:filter-rules>
                     <oc:favorite>1</oc:favorite>
                 </oc:filter-rules>
                </oc:filter-files>`
  return httpHelper
    .report(`files/${user}`, user, body)
    .then(res => res.text())
    .then(res => {
      const favData = convert.xml2js(res, { compact: true })['d:multistatus']['d:response']
      const favItems = []
      favData.map(favourite => {
        favItems.push({
          resource: filename(favourite['d:href']._text),
          isFolder: !(Object.keys(favourite['d:propstat']['d:prop']['d:resourcetype']).length === 0)
        })
      })
      return favItems
    })
}
