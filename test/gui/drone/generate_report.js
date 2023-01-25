// clang-format off
const fs = require('fs')
const path = require('path')
const process = require('process')

function getResultFile() {
  const resultDir = process.argv.pop()

  if (!resultDir) {
    throw new Error('[Error] Report directory path not provided')
  }

  const filename = 'results-v1.js'
  const resultFile = path.join(path.resolve(resultDir), filename)

  if (!fs.existsSync(resultFile)) {
    throw new Error('Report file not found: ' + resultFile)
  }
  return resultFile
}

function parse(file) {
  let content = fs.readFileSync(file, 'utf8')
  content += '\n;(()=> data)();'
  return eval(content)
}

function generateReport(result) {
  const report = {}
  for (const suite of result.tests) {
    for (const tstCase of suite.tests) {
      const test_case_report = []
      for (const feature of tstCase.tests) {
        for (const scenario of feature.tests) {
          // TODO: for scenario outline
          // TODO: check each step for result
          test_case_report.push({
            scenario: scenario.name,
            status: scenario.isSkipped
              ? ':skipped: Skipped'
              : scenario.result ?? ':tick: PASS',
          })
        }
      }
      report[tstCase.name] = test_case_report
    }
  }
  return report
}

function logReport(report) {
  log = ['+' + '-'.repeat(80) + '+']
  for (const tstCase in report) {
    log.push(`| ${tstCase}` + ' '.repeat(66) + '|')
    log.push('+' + '-'.repeat(80) + '+')
    report[tstCase].forEach((test, idx) => {
      log.push(`| ${idx + 1}. | ${test.scenario} | ${test.status} |`)
      log.push('|' + '_'.repeat(80) + '|')
    })
  }
  console.log(log.join('\n'))
  return log.join('\n')
}

function htmlLog(report) {
  log = ['+' + '-'.repeat(80) + '+']
  for (const tstCase in report) {
    log.push(`| ${tstCase}` + ' '.repeat(66) + '|')
    log.push('+' + '-'.repeat(80) + '+')
    report[tstCase].forEach((test, idx) => {
      log.push(`| ${idx + 1}. | ${test.scenario} | ${test.status} |`)
      log.push('|' + '_'.repeat(80) + '|')
    })
  }
  console.log(log.join('\n'))
  return log.join('\n')
}

function parseResult() {
  const resultFile = getResultFile()

  if (!fs.existsSync(resultFile)) {
    throw new Error('Report file not found: ' + resultFile)
  }
  if (!fs.lstatSync(resultFile).isFile()) {
    throw new Error('Provided path is not a file: ' + resultFile)
  }
  if (path.extname(resultFile) !== '.js') {
    // TODO: parse results from the file content
    throw new Error('Provided path is not a JS file: ' + resultFile)
  }

  const results = parse(resultFile)
  for (const result of results) {
    const report = generateReport(result)
    logReport(report)
  }
}

parseResult()
