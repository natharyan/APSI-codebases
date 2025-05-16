import { HttpStatusCodes } from '../enum/StatusCode'
import { CustomAPIError } from './custom-api'

class NotFoundError extends CustomAPIError {
  statusCode: any
  constructor(message: any) {
    super(message)
    this.statusCode = HttpStatusCodes.NOT_FOUND
  }
}

export { NotFoundError }
