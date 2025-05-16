import crypto from 'crypto'

const hashString = (string: any) =>
  crypto.createHash('md5').update(string).digest('hex')

export { hashString }
