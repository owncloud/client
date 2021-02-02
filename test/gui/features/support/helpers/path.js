const join = require('join-path')
const fs = require('fs')
const _ = require('lodash/fp')
const assert = require('assert')
const normalize = _.replace(/^\/+|$/g, '')
const parts = _.pipe(
  normalize,
  _.split('/')
)
const relativeTo = function(basePath, childPath) {
  basePath = normalize(basePath)
  childPath = normalize(childPath)
  assert.ok(childPath.startsWith(basePath), `${childPath} does not contain ${basePath}`)
  const basePathLength = basePath.length
  return childPath.slice(basePathLength)
}
const deleteFolderRecursive = function(path) {
  if (fs.existsSync(path)) {
    fs.readdirSync(path).forEach((file, index) => {
      const curPath = join(path, file)
      if (fs.lstatSync(curPath).isDirectory()) {
        // recurse
        deleteFolderRecursive(curPath)
      } else {
        // delete file
        fs.unlinkSync(curPath)
      }
    })
    fs.rmdirSync(path)
  }
}

module.exports = {
  normalize,
  resolve: _.partial(join, ['/']),
  join,
  parts,
  relativeTo,
  filename: _.pipe(
    parts,
    _.remove(n => n === ''),
    _.last
  ),
  deleteFolderRecursive
}
