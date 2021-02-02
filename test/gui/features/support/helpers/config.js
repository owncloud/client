const httpHelper = require('./httpHelper')
const occHelper = require('./occHelper')
const { difference } = require('./objects')
const _ = require('lodash')
const pLimit = require('p-limit')

// run 10 promises at once at max
const limit = pLimit(10)

const config = {}

async function setSkeletonDirectory(server, admin) {
  const data = JSON.stringify({ directory: 'webUISkeleton' })
  const apiUrl = 'apps/testing/api/v1/testingskeletondirectory'
  const resp = await httpHelper.postOCS(apiUrl, 'admin', data, {
    'Content-Type': 'application/json'
  })

  httpHelper.checkStatus(resp, 'Could not set skeletondirectory.')
}

function rollbackSystemConfigs(oldSysConfig, newSysConfig) {
  const configToChange = difference(newSysConfig, oldSysConfig)
  const _rollbacks = []

  for (const key in configToChange) {
    if (typeof configToChange[key] === 'object') {
      continue
    }
    const value = _.get(oldSysConfig, [key])
    if (value === undefined) {
      _rollbacks.push(limit(occHelper.runOcc, ['config:system:delete', key]))
    } else {
      _rollbacks.push(limit(occHelper.runOcc, ['config:system:set', key, `--value=${value}`]))
    }
  }

  return Promise.all(_rollbacks)
}

function rollbackAppConfigs(oldAppConfig, newAppConfig) {
  const configToChange = difference(newAppConfig, oldAppConfig)

  const _rollbacks = []

  for (const app in configToChange) {
    for (const key in configToChange[app]) {
      const value = _.get(oldAppConfig, [app, key])
      if (value === undefined) {
        _rollbacks.push(limit(occHelper.runOcc, ['config:app:delete', app, key]))
      } else {
        _rollbacks.push(limit(occHelper.runOcc, ['config:app:set', app, key, `--value=${value}`]))
      }
    }
  }

  return Promise.all(_rollbacks)
}

export async function getConfigs() {
  const resp = await occHelper.runOcc(['config:list'])
  let stdOut = _.get(resp, 'ocs.data.stdOut')
  if (stdOut === undefined) {
    throw new Error('stdOut notFound, Found:', resp)
  }
  stdOut = JSON.parse(stdOut)
  return stdOut
}

export async function cacheConfigs(server) {
  config[server] = await getConfigs()
  return config
}

export async function setConfigs(server, admin = 'admin') {
  await setSkeletonDirectory(server, admin)
}

export async function rollbackConfigs(server) {
  const newConfig = await getConfigs()

  const appConfig = _.get(newConfig, 'apps')
  const systemConfig = _.get(newConfig, 'system')

  const initialAppConfig = _.get(config[server], 'apps')
  const initialSysConfig = _.get(config[server], 'system')

  await Promise.all([
    rollbackSystemConfigs(initialSysConfig, systemConfig),
    rollbackAppConfigs(initialAppConfig, appConfig)
  ])
}
