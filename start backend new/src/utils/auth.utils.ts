
import bcrypt from 'bcryptjs'

const comparePassword = async function (clientPassword: string, serverPassword: string) {
    const isMatch = await bcrypt.compare(clientPassword, serverPassword)
    return isMatch
}

export { comparePassword }